#include "PlayerComponent.h"
#include "Input.h"
#include "GameCamera.h"
#include <algorithm>
#include <cmath>
#include <random>   // ランダムな左右オフセット生成に使用
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"
#include "../Bullet/BulletComponent.h"
#include "../Bullet/HomingBulletComponent.h"

void PlayerComponent::Initialize() {
  cursorPos_ = { 640.0f, 360.0f }; // 画面中央
  localPos_ = { 0.0f, -0.5f }; // 初期位置：少し下側（前方を見やすく）
  hp_ = maxHp_;
}

void PlayerComponent::Update(float deltaTime) {
  if (!owner_) return;
  auto scene = BaseScene::GetActiveScene();
  if (!scene) return;
  auto* input = scene->GetInput();
  auto* camera = scene->GetMainCamera();
  if (!input || !camera) return;

  // 入力によるカーソルの移動
  float cStep = cursorSpeed_ * deltaTime;
  if (input->PressKey(DIK_W) || input->PressKey(DIK_UP)) {
    cursorPos_.y -= cStep; // スクリーン座標なので上は-
  }
  if (input->PressKey(DIK_S) || input->PressKey(DIK_DOWN)) {
    cursorPos_.y += cStep;
  }
  if (input->PressKey(DIK_A) || input->PressKey(DIK_LEFT)) {
    cursorPos_.x -= cStep;
  }
  if (input->PressKey(DIK_D) || input->PressKey(DIK_RIGHT)) {
    cursorPos_.x += cStep;
  }

  // 画面端に行き過ぎないようにカーソルをクランプ (HUDで見えなくなるのを防ぐ)
  cursorPos_.x = std::clamp(cursorPos_.x, 80.0f, 1200.0f);
  cursorPos_.y = std::clamp(cursorPos_.y, 60.0f, 660.0f);

  // カーソル座標から、自機が向かうべき目標のローカル座標を計算
  // X: 0~1280 -> -3.5~3.5, Y: 0~720 -> 1.8~-1.8 (上方向が+になるように反転)
  Vector2 targetLocalPos = {
      (cursorPos_.x - 640.0f) * (3.5f / 640.0f),
      -(cursorPos_.y - 360.0f) * (1.8f / 360.0f)
  };

  // 目標座標に向かってLerp（滑らかに追従）
  localPos_.x += (targetLocalPos.x - localPos_.x) * moveSpeed_ * deltaTime;
  localPos_.y += (targetLocalPos.y - localPos_.y) * moveSpeed_ * deltaTime;

  // カメラの情報を取得
  Vector3 eye = camera->GetEye();
  Vector3 forward = camera->GetForward();
  Vector3 right = camera->GetRight();
  Vector3 up = camera->GetActualUp();

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

  // ロックオンシステムの更新
  lockon_.Update(deltaTime, input, camera, scene, cursorPos_);

  // クールダウンの減算（以前消えてしまっていたため復活）
  if (shootCooldown_ > 0.0f) {
      shootCooldown_ -= deltaTime;
  }

  // 弾の発射関連
  bool isReleased = input->ReleaseKey(DIK_SPACE) || input->WasPadReleased(XINPUT_GAMEPAD_A) || input->WasMouseReleased(0);

  if (isReleased) {
      if (lockon_.IsLockingMode()) {
          // 1. 長押し（ロックオンモード）から離したとき：ホーミング弾を発射
          const auto& targets = lockon_.GetLockedTargets();
          if (!targets.empty()) {
              // ランダム左右オフセット生成用（弾ごとに異なる弧を描かせる）
              static std::mt19937 rng(std::random_device{}());
              std::uniform_real_distribution<float> sideDist(-6.0f, 6.0f);   // 左右オフセット幅
              std::uniform_real_distribution<float> upDist(5.0f, 10.0f);     // 上方向オフセット幅

              for (const auto& weakTarget : targets) {
                  // lock()で生存確認。既に消滅しているターゲットへの発射を防ぐ
                  auto target = weakTarget.lock();
                  if (!target) continue;

                  auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("PlayerHomingBullet");
                  bulletObj->SetTag("PlayerBullet");

                  auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
                  bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
                  bulletObj->AddComponent(std::move(bulletModelComp));

                  auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
                  colliderComp->type = AbsoluteEngine::ColliderComponent::Type::Sphere;
                  colliderComp->radius = 1.0f; // ホーミング弾は当たり判定を広めに設定
                  bulletObj->AddComponent(std::move(colliderComp));

                  // --- ベジェ曲線用パラメータの計算 ---

                  // P0: 始点（現在の銃口ワールド座標）
                  Vector3 p0 = t.translate;
                  bulletObj->GetTransform().translate = p0;

                  // P2の初期値: ターゲットの現在座標（発射瞬間）
                  Vector3 p2Initial = target->GetTransform().translate;

                  // 中間点: P0 と P2_initial の中点
                  Vector3 midPoint = {
                      (p0.x + p2Initial.x) * 0.5f,
                      (p0.y + p2Initial.y) * 0.5f,
                      (p0.z + p2Initial.z) * 0.5f
                  };

                  // P1: 中間点から上方向 + ランダムな左右オフセットを加えた制御点
                  // 弾ごとに異なるランダム値にすることで、花火のように散開した放物線を実現する
                  float sideOffset = sideDist(rng); // ランダム左右オフセット
                  float upOffset   = upDist(rng);   // ランダム上方オフセット

                  // カメラのRight方向を使って「横」を定義することで、どの方向を見ていても正しく散開する
                  Vector3 p1 = {
                      midPoint.x + right.x * sideOffset,
                      midPoint.y + upOffset,
                      midPoint.z + right.z * sideOffset
                  };

                  // 着弾時間は固定（0.6秒）
                  constexpr float kBulletDuration = 0.6f;

                  // HomingBulletComponentをベジェ曲線パラメータで初期化
                  auto homingComp = std::make_unique<HomingBulletComponent>();
                  homingComp->Initialize(p0, p1, kBulletDuration, weakTarget);
                  bulletObj->AddComponent(std::move(homingComp));

                  scene->AddRootObject(bulletObj);
              }
          }
      } else {
          // 2. 短押し（ロックオンモードに入る前）で離したとき：通常弾を発射
          if (shootCooldown_ <= 0.0f) {
              shootCooldown_ = 0.15f; // 連射速度を適正化
              auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("PlayerBullet");
              bulletObj->SetTag("PlayerBullet");
              
              auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
              bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
              bulletObj->AddComponent(std::move(bulletModelComp));
              
              auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
              colliderComp->type = AbsoluteEngine::ColliderComponent::Type::Sphere;
              colliderComp->radius = 0.5f;
              bulletObj->AddComponent(std::move(colliderComp));
              
              bulletObj->GetTransform().translate = t.translate;
              
              // カーソル位置をNDC座標（-1.0〜1.0）に変換
              float ndcX = (cursorPos_.x / 1280.0f) * 2.0f - 1.0f;
              float ndcY = 1.0f - (cursorPos_.y / 720.0f) * 2.0f;
              
              // プロジェクション座標での遠平面上の点
              Vector3 ndcFar = { ndcX, ndcY, 1.0f };
              
              // ViewProjectionの逆行列を使ってワールド座標に変換
              Matrix4x4 vpMat = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());
              Matrix4x4 invVp = Inverse(vpMat);
              
              float w = ndcFar.x * invVp.m[0][3] + ndcFar.y * invVp.m[1][3] + ndcFar.z * invVp.m[2][3] + invVp.m[3][3];
              Vector3 worldTarget = {
                  (ndcFar.x * invVp.m[0][0] + ndcFar.y * invVp.m[1][0] + ndcFar.z * invVp.m[2][0] + invVp.m[3][0]) / w,
                  (ndcFar.x * invVp.m[0][1] + ndcFar.y * invVp.m[1][1] + ndcFar.z * invVp.m[2][1] + invVp.m[3][1]) / w,
                  (ndcFar.x * invVp.m[0][2] + ndcFar.y * invVp.m[1][2] + ndcFar.z * invVp.m[2][2] + invVp.m[3][2]) / w
              };
              
              // 自機位置から目標地点への方向ベクトル
              Vector3 diff = { worldTarget.x - t.translate.x, worldTarget.y - t.translate.y, worldTarget.z - t.translate.z };
              float len = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
              if (len > 0.0001f) {
                  diff.x /= len;
                  diff.y /= len;
                  diff.z /= len;
              } else {
                  diff = forward;
              }
              
              Vector3 vel = { diff.x * 45.0f, diff.y * 45.0f, diff.z * 45.0f }; // 弾速
              auto bulletComp = std::make_unique<BulletComponent>();
              bulletComp->Initialize(vel);
              bulletObj->AddComponent(std::move(bulletComp));
              
              scene->AddRootObject(bulletObj);
          }
      }
      // いずれの場合も発射後はロックオン状態をリセット
      lockon_.Initialize();
  }
}

void PlayerComponent::TakeDamage(int damage) {
  hp_ -= damage;
  if (hp_ < 0) hp_ = 0;
}

bool PlayerComponent::IsDead() const {
  return hp_ <= 0;
}

void PlayerComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (IsDead()) return;

    if (other->GetName().find("Enemy") != std::string::npos || other->GetTag() == "Enemy" || 
        other->GetName().find("Boss") != std::string::npos || other->GetTag() == "Boss") {
        TakeDamage(1);
    }
}