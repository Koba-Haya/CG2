#include "PlayerComponent.h"
#include "Input.h"
#include "GameCamera.h"
#include <algorithm>
#include <cmath>
#include <random>
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"
#include "AbsoluteEngine/scene/RigidbodyComponent.h"
#include "AbsoluteEngine/resources/AssetManager.h"
#include "ModelResource.h"
#include "ParticleManager.h"
#include "../Bullet/BulletComponent.h"
#include "../Bullet/HomingBulletComponent.h"

// 武器（右手に持たせる仮モデル）の右手ボーン名
static const char* const kRightHandBoneName = "mixamorig:RightHand";

// レティクルの距離設定（パンツァードラグーン風3重レティクル）
static constexpr float kNearReticleDist  = 10.0f;  // 近レティクル
static constexpr float kMidReticleDist   = 30.0f;  // 中レティクル（自機同期）
static constexpr float kFarReticleDist   = 60.0f;  // 遠レティクル（弾道の照準）

// バンク角の最大値・補間速度（スターフォックス64風：素早く傾き、素早く戻る）
static constexpr float kMaxBankRoll  = 0.6f;  // 最大ロール角（ラジアン）：機敏な動きに合わせて強調
static constexpr float kMaxBankPitch = 0.15f; // 最大ピッチ追加角（ラジアン）
static constexpr float kBankSpeed    = 8.0f;  // バンク補間速度：素早く傾き、素早く戻るように倍速

void PlayerComponent::Initialize() {
  cursorPos_ = { 640.0f, 360.0f };
  localPos_  = { 0.0f, -0.5f };
  hp_        = maxHp_;
  bankRoll_  = 0.0f;
  bankPitch_ = 0.0f;

  // 武器モデル（仮：cube.objを細長くスケールして武器として持たせる）の生成
  if (!weaponModel_) {
      auto* am = AbsoluteEngine::AssetManager::GetInstance();
      auto weaponRes = am->Load<ModelResource>("resources/app/cube/cube.obj");
      if (weaponRes) {
          weaponModel_ = std::make_unique<ModelInstance>();
          ModelInstance::CreateInfo ci{};
          ci.resource = weaponRes;
          // 明るく目立つ色にする（レールシューティングでカメラが自機から離れているため、
          // 地味な灰色だと視認しづらい）
          ci.baseColor = { 0.95f, 0.25f, 0.15f, 1.0f };
          weaponModel_->Initialize(ci);
      }
  }
}

void PlayerComponent::Update(float deltaTime) {
  if (!owner_) return;
  auto scene  = BaseScene::GetActiveScene();
  if (!scene) return;
  auto* input  = scene->GetInput();
  auto* camera = scene->GetMainCamera();
  if (!input || !camera) return;

  // -----------------------------------------------------------------------
  // 自機の RigidbodyComponent が存在する場合は重力を無効化する
  // （物理演算によって自機が落下するバグの解消）
  // -----------------------------------------------------------------------
  if (auto rb = owner_->GetComponent<AbsoluteEngine::RigidbodyComponent>()) {
      rb->SetUseGravity(false);
      rb->SetKinematic(true); // 物理演算の一切を無効化して完全手動制御にする
  }

  // -----------------------------------------------------------------------
  // 入力によるカーソル移動
  // -----------------------------------------------------------------------
  float cStep = cursorSpeed_ * deltaTime;

  float inputX = 0.0f;
  float inputY = 0.0f;

  if (input->PressKey(DIK_W) || input->PressKey(DIK_UP))    { inputY -= 1.0f; cursorPos_.y -= cStep; }
  if (input->PressKey(DIK_S) || input->PressKey(DIK_DOWN))  { inputY += 1.0f; cursorPos_.y += cStep; }
  if (input->PressKey(DIK_A) || input->PressKey(DIK_LEFT))  { inputX -= 1.0f; cursorPos_.x -= cStep; }
  if (input->PressKey(DIK_D) || input->PressKey(DIK_RIGHT)) { inputX += 1.0f; cursorPos_.x += cStep; }

  // ゲームパッド左スティック入力も反映
  if (input->IsGamepadConnected()) {
      auto pad = input->GetGamepad();
      float padX = pad.lx / 32767.0f;
      float padY = -pad.ly / 32767.0f;
      const float deadzone = 0.2f;
      if (std::abs(padX) > deadzone) { inputX = padX; cursorPos_.x += padX * cStep; }
      if (std::abs(padY) > deadzone) { inputY = padY; cursorPos_.y += padY * cStep; }
  }

  // カーソルのクランプ（画面全体を可動域とし、端まで届くように設定）
  cursorPos_.x = std::clamp(cursorPos_.x, 0.0f, 1280.0f);
  cursorPos_.y = std::clamp(cursorPos_.y, 0.0f,  720.0f);

  // -----------------------------------------------------------------------
  // バンク角（ロール・ピッチ）の計算
  // 左右移動でロール、上下移動でピッチ上乗せ。原作ライクな機体傾きを実現。
  // -----------------------------------------------------------------------
  float targetRoll  = -inputX * kMaxBankRoll;  // 右移動で右傾き（マイナス）
  float targetPitch =  inputY * kMaxBankPitch; // 上移動でピッチ下（前傾き）

  bankRoll_  += (targetRoll  - bankRoll_)  * kBankSpeed * deltaTime;
  bankPitch_ += (targetPitch - bankPitch_) * kBankSpeed * deltaTime;

  // -----------------------------------------------------------------------
  // カーソル座標 → 自機の目標ローカル座標
  //
  // 固定係数（6.0f 等のマジックナンバー）を廃止し、カメラの射線（camDir）と
  // cameraDistance_ を使って「カーソルが画面上で指している3D位置」を正確に逆算する。
  // これにより、カーソルが画面端にあれば自機も画面端まで移動でき、
  // cameraDistance_ を変更しても可動域が自動的にぴったり一致し続ける。
  // -----------------------------------------------------------------------

  // カメラ基本ベクトルを先に取得（後続のすべての計算で共用する）
  Vector3 eye     = camera->GetEye();
  Vector3 forward = camera->GetForward();
  Vector3 right   = camera->GetRight();
  Vector3 up      = camera->GetActualUp();

  // NDC 座標に変換
  float ndcX_move = (cursorPos_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY_move = 1.0f - (cursorPos_.y / 720.0f) * 2.0f;

  // カメラのビュープロジェクション行列の逆行列を計算
  Matrix4x4 vpMat_move = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());
  Matrix4x4 invVp_move = Inverse(vpMat_move);

  // NDC → ワールド座標変換ヘルパー
  auto unprojectMove = [&](float z) -> Vector3 {
      Vector3 ndc = { ndcX_move, ndcY_move, z };
      float w = ndc.x * invVp_move.m[0][3] + ndc.y * invVp_move.m[1][3] + ndc.z * invVp_move.m[2][3] + invVp_move.m[3][3];
      return {
          (ndc.x * invVp_move.m[0][0] + ndc.y * invVp_move.m[1][0] + ndc.z * invVp_move.m[2][0] + invVp_move.m[3][0]) / w,
          (ndc.x * invVp_move.m[0][1] + ndc.y * invVp_move.m[1][1] + ndc.z * invVp_move.m[2][1] + invVp_move.m[3][1]) / w,
          (ndc.x * invVp_move.m[0][2] + ndc.y * invVp_move.m[1][2] + ndc.z * invVp_move.m[2][2] + invVp_move.m[3][2]) / w
      };
  };

  // カーソル方向の正規化されたカメラ射線ベクトルを計算
  Vector3 camNear_move = unprojectMove(0.0f);
  Vector3 camFar_move  = unprojectMove(1.0f);
  Vector3 camDir_move = {
      camFar_move.x - camNear_move.x,
      camFar_move.y - camNear_move.y,
      camFar_move.z - camNear_move.z
  };
  float camDirLen_move = std::sqrt(camDir_move.x*camDir_move.x + camDir_move.y*camDir_move.y + camDir_move.z*camDir_move.z);
  if (camDirLen_move > 0.0001f) {
      camDir_move.x /= camDirLen_move;
      camDir_move.y /= camDirLen_move;
      camDir_move.z /= camDirLen_move;
  } else {
      camDir_move = forward;
  }

  Vector3 eye_move    = eye;
  Vector3 targetMoveWorldPos = {
      eye_move.x + camDir_move.x * cameraDistance_,
      eye_move.y + camDir_move.y * cameraDistance_,
      eye_move.z + camDir_move.z * cameraDistance_
  };

  // ワールド座標をローカル座標（Right/Up 軸上の射影）に変換して追従目標に設定
  Vector3 right_move = right;
  Vector3 up_move    = up;
  Vector3 eye_fwd    = forward;
  Vector3 basePos = {
      eye.x + eye_fwd.x * cameraDistance_,
      eye.y + eye_fwd.y * cameraDistance_,
      eye.z + eye_fwd.z * cameraDistance_
  };
  Vector3 delta = {
      targetMoveWorldPos.x - basePos.x,
      targetMoveWorldPos.y - basePos.y,
      targetMoveWorldPos.z - basePos.z
  };
  float localTargetX = delta.x * right_move.x + delta.y * right_move.y + delta.z * right_move.z;
  float localTargetY = delta.x * up_move.x    + delta.y * up_move.y    + delta.z * up_move.z;

  // 目標座標にLerpで滑らかに追従
  localPos_.x += (localTargetX - localPos_.x) * moveSpeed_ * deltaTime;
  localPos_.y += (localTargetY - localPos_.y) * moveSpeed_ * deltaTime;

  // ワールド座標の計算: Eye + Forward*Dist + Right*localX + Up*localY
  // （カメラベクトルは関数先頭で取得済みの変数を再利用）
  // -----------------------------------------------------------------------

  Vector3 worldPos = {
      eye.x + forward.x * cameraDistance_ + right.x * localPos_.x + up.x * localPos_.y,
      eye.y + forward.y * cameraDistance_ + right.y * localPos_.x + up.y * localPos_.y,
      eye.z + forward.z * cameraDistance_ + right.z * localPos_.x + up.z * localPos_.y
  };

  auto& t = owner_->GetTransform();
  t.translate = worldPos;

  // -----------------------------------------------------------------------
  // 回転の計算：カメラ向き（ヨー・ピッチ）＋バンク角（ロール・ピッチ追加）
  // -----------------------------------------------------------------------
  float yaw   = std::atan2(forward.x, forward.z);
  float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
  float pitch = std::atan2(-forward.y, xzLen);

  // ピッチにバンクピッチを加算、ロールをバンクロールに設定
  t.rotate = { pitch + bankPitch_, yaw, bankRoll_ };

  // -----------------------------------------------------------------------
  // 3D レティクルのワールド座標を計算・キャッシュする（射線パース方式・修正版）
  //
  // 【旧実装の問題点】
  //   Farレティクルを「カメラ起点の射線上の固定距離（60m）点」に置いていたため、
  //   自機の位置が前後に動くと、自機→固定点の方向がカメラ射線からずれてしまい、
  //   レティクルのパース（傾き）が逆向きに見える現象が起きていた。
  //
  // 【新実装の方針】「レーザー（自機→ターゲット）線分」を基準にする
  //   STEP 1: カメラ起点の射線で「3D空間上のターゲット点（targetWorldPos）」を算出。
  //           → カーソル方向へのカメラ射線と、遠クリップ面との交点を使う。
  //             「遠点」は kFarReticleDist の固定距離に依存せず、
  //              NDCのz=1面を逆投影した方向ベクトルで無限遠方向を表現し、
  //              farReticleWorldPos_ には targetWorldPos をそのまま代入する。
  //   STEP 2: 自機（t.translate）→ targetWorldPos を線分とし、
  //           Lerp（割合指定）で Near/Mid をその線分上に配置。
  //
  // これにより、どの位置から見ても奥のレティクルが手前のレティクルに向かって
  // 正しく「内側に絞り込む」パース表現が得られる。
  // -----------------------------------------------------------------------
  {
      // NDC 座標に変換
      float ndcX = (cursorPos_.x / 1280.0f) * 2.0f - 1.0f;
      float ndcY = 1.0f - (cursorPos_.y / 720.0f) * 2.0f;

      // ビュープロジェクション逆行列を計算
      Matrix4x4 vpMat = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());
      Matrix4x4 invVp = Inverse(vpMat);

      // NDC座標をワールド座標に逆変換するヘルパー
      auto unproject = [&](float z) -> Vector3 {
          Vector3 ndc = { ndcX, ndcY, z };
          float w = ndc.x * invVp.m[0][3] + ndc.y * invVp.m[1][3] + ndc.z * invVp.m[2][3] + invVp.m[3][3];
          return {
              (ndc.x * invVp.m[0][0] + ndc.y * invVp.m[1][0] + ndc.z * invVp.m[2][0] + invVp.m[3][0]) / w,
              (ndc.x * invVp.m[0][1] + ndc.y * invVp.m[1][1] + ndc.z * invVp.m[2][1] + invVp.m[3][1]) / w,
              (ndc.x * invVp.m[0][2] + ndc.y * invVp.m[1][2] + ndc.z * invVp.m[2][2] + invVp.m[3][2]) / w
          };
      };

      // カーソル方向射線を正規化して求め、メンバ変数にキャッシュする
      // （弾の発射方向をここで確定させることで、t.translateのLerp遅れの影響を受けない）
      Vector3 camNearWorld = unproject(0.0f);
      Vector3 camFarWorld  = unproject(1.0f);
      Vector3 camDir = {
          camFarWorld.x - camNearWorld.x,
          camFarWorld.y - camNearWorld.y,
          camFarWorld.z - camNearWorld.z
      };
      float camDirLen = std::sqrt(camDir.x*camDir.x + camDir.y*camDir.y + camDir.z*camDir.z);
      if (camDirLen > 0.0001f) {
          camDir.x /= camDirLen; camDir.y /= camDirLen; camDir.z /= camDirLen;
      } else {
          camDir = forward;
      }
      // 毎フレームキャッシュ：弾の発射方向に直接使用する
      camDirCached_ = camDir;

      // -----------------------------------------------------------------------
      // 【修正】Near / Mid / Far をすべて「自機位置（t.translate）起点」に統一
      //
      // 【旧実装の問題点（バグの原因）】
      //   Far は eye（カメラ）起点、Near/Mid は t.translate（自機）起点と
      //   始点がバラバラだったため、カーソル移動時に Far がほとんど動かず
      //   Near が最も大きく動くという逆転現象が起きていた。
      //
      // 【新実装】
      //   3つすべてを「自機位置（t.translate）」を起点とし、
      //   同じ camDir 方向へ距離定数分だけ伸ばして配置する。
      //   遠近法による画面上の視差は WorldToScreen（透視変換）が自動的に処理する。
      //   → Near（大・手前）: 画面上の移動量が大、Far（小・奥）: 移動量が小 ← 正しい遠近感
      // -----------------------------------------------------------------------

      // 近レティクル：自機から camDir 方向へ kNearReticleDist m
      nearReticleWorldPos_ = {
          t.translate.x + camDir.x * kNearReticleDist,
          t.translate.y + camDir.y * kNearReticleDist,
          t.translate.z + camDir.z * kNearReticleDist
      };
      // 中レティクル：自機から camDir 方向へ kMidReticleDist m
      midReticleWorldPos_ = {
          t.translate.x + camDir.x * kMidReticleDist,
          t.translate.y + camDir.y * kMidReticleDist,
          t.translate.z + camDir.z * kMidReticleDist
      };
      // 遠レティクル：自機から camDir 方向へ kFarReticleDist m（弾道照準・レーザー終点）
      farReticleWorldPos_ = {
          t.translate.x + camDir.x * kFarReticleDist,
          t.translate.y + camDir.y * kFarReticleDist,
          t.translate.z + camDir.z * kFarReticleDist
      };
  }

  // -----------------------------------------------------------------------
  // ロックオン・弾発射はPlayMode::Playのときのみ処理する
  // （タイムラインのエディットモードプレビュー（レールカメラのシーク同期）でも
  //   このUpdate()は位置合わせのために呼ばれるため、ここをガードしないと
  //   Playモードにしていないのにクリック/スペース入力で実弾発射が走ってしまう）
  // -----------------------------------------------------------------------
  if (scene->GetPlayMode() != PlayMode::Play) return;

  // ロックオンシステムの更新
  lockon_.Update(deltaTime, input, camera, scene, cursorPos_);

  // クールダウンの減算
  if (shootCooldown_ > 0.0f) {
      shootCooldown_ -= deltaTime;
  }

  // -----------------------------------------------------------------------
  // 弾の発射位置：右手ボーンのワールド座標（Skeletonが無い場合は自機中心にフォールバック）
  // -----------------------------------------------------------------------
  Vector3 handPos = t.translate;
  if (auto* modelComp = owner_->GetComponent<AbsoluteEngine::ModelComponent>()) {
      if (auto* instance = modelComp->GetModelInstance()) {
          Matrix4x4 handWorld = instance->GetBoneWorldMatrix(kRightHandBoneName);
          handPos = { handWorld.m[3][0], handWorld.m[3][1], handWorld.m[3][2] };
      }
  }

  // -----------------------------------------------------------------------
  // 弾の発射
  // -----------------------------------------------------------------------
  bool isReleased = input->ReleaseKey(DIK_SPACE)
                 || input->WasPadReleased(XINPUT_GAMEPAD_A)
                 || input->WasMouseReleased(0);

  if (isReleased) {
      // 手からのマズルフラッシュ（パーティクル）
      {
          static std::mt19937 muzzleRng(std::random_device{}());
          std::uniform_real_distribution<float> muzzleDist(-1.0f, 1.0f);
          for (int i = 0; i < 6; ++i) {
              Vector3 vel = {
                  muzzleDist(muzzleRng) * 3.0f,
                  muzzleDist(muzzleRng) * 3.0f,
                  muzzleDist(muzzleRng) * 3.0f
              };
              // レールシューティングでカメラが自機から離れているため、
              // 元の scale 0.3 / lifeTime 0.15 だと画面上でほぼ視認できなかった。
              // 視認性を上げるためスケールと寿命を引き上げる。
              ParticleManager::GetInstance()->Emit(
                  "default", handPos, vel,
                  Vector3{ 0.6f, 0.6f, 0.6f }, Vector3{ 0, 0, 0 }, 0.3f,
                  Vector4{ 1.0f, 0.9f, 0.4f, 1.0f });
          }
      }

      if (lockon_.IsLockingMode()) {
          // ロックオンモード → ホーミング弾（ベジェ曲線）を全ターゲットに発射
          const auto& targets = lockon_.GetLockedTargets();
          if (!targets.empty()) {
              static std::mt19937 rng(std::random_device{}());
              std::uniform_real_distribution<float> sideDist(-6.0f, 6.0f);
              std::uniform_real_distribution<float> upDist(5.0f, 10.0f);

              for (const auto& weakTarget : targets) {
                  auto target = weakTarget.lock();
                  if (!target) continue;

                  auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("PlayerHomingBullet");
                  bulletObj->SetTag("PlayerBullet");

                  auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
                  bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
                  bulletObj->AddComponent(std::move(bulletModelComp));

                  auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
                  colliderComp->type   = AbsoluteEngine::ColliderComponent::Type::Sphere;
                  colliderComp->radius = 1.0f;
                  bulletObj->AddComponent(std::move(colliderComp));

                  Vector3 p0 = handPos;
                  bulletObj->GetTransform().translate = p0;

                  Vector3 p2Initial = target->GetTransform().translate;
                  Vector3 midPoint  = { (p0.x + p2Initial.x) * 0.5f, (p0.y + p2Initial.y) * 0.5f, (p0.z + p2Initial.z) * 0.5f };

                  float sideOffset = sideDist(rng);
                  float upOffset   = upDist(rng);
                  Vector3 p1 = {
                      midPoint.x + right.x * sideOffset,
                      midPoint.y + upOffset,
                      midPoint.z + right.z * sideOffset
                  };

                  constexpr float kBulletDuration = 0.6f;
                  auto homingComp = std::make_unique<HomingBulletComponent>();
                  homingComp->Initialize(p0, p1, kBulletDuration, weakTarget);
                  bulletObj->AddComponent(std::move(homingComp));

                  scene->AddRootObject(bulletObj);
              }
          }
      } else {
          // 通常弾：farReticleWorldPos_ に向けて発射（タスク4）
          if (shootCooldown_ <= 0.0f) {
              shootCooldown_ = 0.15f;

              auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("PlayerBullet");
              bulletObj->SetTag("PlayerBullet");

              auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
              bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
              bulletObj->AddComponent(std::move(bulletModelComp));

              auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
              colliderComp->type   = AbsoluteEngine::ColliderComponent::Type::Sphere;
              colliderComp->radius = 0.5f;
              bulletObj->AddComponent(std::move(colliderComp));

              bulletObj->GetTransform().translate = handPos;

              // 弾道：camDirCached_（最新カーソル方向）を直接使用
              // ※ farReticleWorldPos_ - t.translate の差分計算では
              //   t.translate の Lerp 遅れが方向ズレを引き起こすため廃止
              constexpr float kBulletSpeed = 50.0f;
              Vector3 vel = {
                  camDirCached_.x * kBulletSpeed,
                  camDirCached_.y * kBulletSpeed,
                  camDirCached_.z * kBulletSpeed
              };

              auto bulletComp = std::make_unique<BulletComponent>();
              bulletComp->Initialize(vel);
              bulletObj->AddComponent(std::move(bulletComp));

              scene->AddRootObject(bulletObj);
          }
      }
      lockon_.Initialize();
  }
}

void PlayerComponent::Draw() {
  if (!owner_ || !weaponModel_) return;

  auto* modelComp = owner_->GetComponent<AbsoluteEngine::ModelComponent>();
  if (!modelComp) return;
  auto* instance = modelComp->GetModelInstance();
  if (!instance) return;

  // 武器（仮モデル）を毎フレーム右手ボーンに追従させる。
  // cube.objをそのままだと武器らしくないため、細長くスケールしたオフセットを掛けて持たせる。
  // （元は0.3/0.3/1.5だったが、レールシューティングのカメラ距離では小さすぎて
  //   視認できなかったため一回り大きくしている）
  Matrix4x4 handWorld = instance->GetBoneWorldMatrix(kRightHandBoneName);
  Matrix4x4 offset = MakeAffineMatrix({ 0.4f, 0.4f, 2.0f }, Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.7f });
  weaponModel_->SetWorld(Multiply(offset, handWorld));
  weaponModel_->Draw();
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
        other->GetName().find("Boss")  != std::string::npos || other->GetTag() == "Boss") {
        TakeDamage(1);
    }
}