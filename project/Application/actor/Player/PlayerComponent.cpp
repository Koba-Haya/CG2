#include "PlayerComponent.h"
#include "Input.h"
#include "GameCamera.h"
#include <algorithm>
#include <cmath>
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "../Bullet/BulletComponent.h"

void PlayerComponent::Initialize() {
  localPos_ = { 0.0f, -1.0f }; // 初期位置：少し下側
  hp_ = maxHp_;
}

void PlayerComponent::Update(float deltaTime) {
  if (!owner_ || !input_ || !camera_) return;

  // 入力によるローカル座標の移動
  float step = moveSpeed_ * deltaTime;
  if (input_->PressKey(DIK_W) || input_->PressKey(DIK_UP)) {
    localPos_.y += step;
  }
  if (input_->PressKey(DIK_S) || input_->PressKey(DIK_DOWN)) {
    localPos_.y -= step;
  }
  if (input_->PressKey(DIK_A) || input_->PressKey(DIK_LEFT)) {
    localPos_.x -= step;
  }
  if (input_->PressKey(DIK_D) || input_->PressKey(DIK_RIGHT)) {
    localPos_.x += step;
  }

  // 画面外に出ないようにクランプ
  localPos_.x = std::clamp(localPos_.x, -3.5f, 3.5f);
  localPos_.y = std::clamp(localPos_.y, -1.8f, 1.8f);

  // カメラの情報を取得
  Vector3 eye = camera_->GetEye();
  Vector3 forward = camera_->GetForward();
  Vector3 right = camera_->GetRight();
  Vector3 up = camera_->GetActualUp();

  // ワールド座標の計算: Eye + Forward*Dist + Right*localX + Up*localY
  Vector3 forwardOffset = { forward.x * cameraDistance_, forward.y * cameraDistance_, forward.z * cameraDistance_ };
  Vector3 rightOffset = { right.x * localPos_.x, right.y * localPos_.x, right.z * localPos_.x };
  Vector3 upOffset = { up.x * localPos_.y, up.y * localPos_.y, up.z * localPos_.y };

  Vector3 worldPos = {
      eye.x + forwardOffset.x + rightOffset.x + upOffset.x,
      eye.y + forwardOffset.y + rightOffset.y + upOffset.y,
      eye.z + forwardOffset.z + rightOffset.z + upOffset.z
  };

  auto& t = owner_->GetTransform();
  t.translate = worldPos;

  // カメラの向いている方向からオイラー角を求めて回転に適用する
  float yaw = std::atan2(forward.x, forward.z);
  float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
  float pitch = std::atan2(-forward.y, xzLen);
  
  t.rotate = { pitch, yaw, 0.0f };

  // 弾の発射（スペースキー）
  if (shootCooldown_ > 0.0f) shootCooldown_ -= deltaTime;
  if (input_->PressKey(DIK_SPACE) && shootCooldown_ <= 0.0f) {
      shootCooldown_ = 0.25f; // 連射速度を適正化

      auto scene = BaseScene::GetActiveScene();
      if (scene) {
          auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("Bullet");
          
          auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
          bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
          bulletObj->AddComponent(std::move(bulletModelComp));
          
          bulletObj->GetTransform().translate = t.translate;
          
          Vector3 vel = { forward.x * 35.0f, forward.y * 35.0f, forward.z * 35.0f }; // 弾速を適正化
          auto bulletComp = std::make_unique<BulletComponent>();
          bulletComp->Initialize(vel);
          bulletObj->AddComponent(std::move(bulletComp));
          
          scene->AddRootObject(bulletObj);
      }
  }
}

void PlayerComponent::TakeDamage(int damage) {
  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }
}

bool PlayerComponent::IsDead() const {
  return hp_ <= 0;
}