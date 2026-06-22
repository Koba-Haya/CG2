#include "Player/Player.h"
#include "Input.h"
#include "GameCamera.h"
#include <algorithm>
#include <cmath>

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