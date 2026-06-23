#define NOMINMAX
#include "GameScene.h"
#include "Renderer.h"
#include "DirectXCommon.h"
#include "DirectXResourceUtils.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Input.h"
#include "../camera/RailCameraController.h"
#include "Spline.h"
#include "Method.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include "AbsoluteEngine/scene/ComponentFactory.h"
#include "AbsoluteEngine/editor/EditorUIManager.h"
#include "../component/enemy/StraightMoveComponent.h"
#include "../component/enemy/EnemyComponent.h"
#include "../component/enemy/EnemyShootComponent.h"
#include "../component/enemy/BossComponent.h"
#include "SceneIds.h"

// テスト用コンポーネント
class SpinComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (owner_) {
            auto& t = owner_->GetTransform();
            t.rotate.y += 2.0f * deltaTime;
        }
    }
    std::string GetTypeName() const override { return "SpinComponent"; }
};

class MoveComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (owner_) {
            auto& t = owner_->GetTransform();
            t.translate.x += std::sin(frame_ * 0.05f) * 0.05f;
            frame_ += 1.0f;
        }
    }
    std::string GetTypeName() const override { return "MoveComponent"; }
private:
    float frame_ = 0.0f;
};

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  auto* mm = ModelManager::GetInstance();

  // リソースのロード
  resPlayer_ = mm->Load("resources/app/player/player.obj"); // 追加された自機モデル
  resBullet_ = mm->Load("resources/app/bullet/bullet.obj"); // 追加された弾モデル
  resEnemy_  = mm->Load("resources/app/cube/cube.obj"); // 敵モデル
  resEffect_ = mm->Load("resources/app/particle/particle.obj");

  if (!resBullet_) {
      // 万一 bullet.obj が読み込めない場合は、確実に存在する enemy (cube.obj) を仮割り当てする
      resBullet_ = resEnemy_;
  }

  skybox_.Initialize("resources/app/dds/dds.dds");

  // パーティクルグループの初期化（連射ヒット時のアサートクラッシュ防止）
  ParticleManager::GetInstance()->CreateParticleGroup("default", "resources/app/particle/circle.png", 500);

  // レールカメラの初期化
  gameCamera_ = std::make_unique<GameCamera>();
  gameCamera_->Initialize();
  gameCamera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);

  // デバッグカメラ初期化
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize();
  debugCamera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);

  auto railController = std::make_unique<RailCameraController>();
  railController_ = railController.get();
  CameraContext ctx{};
  ctx.deltaTime = 1.0f / 60.0f;
  gameCamera_->SetController(std::move(railController), ctx);
  // シーン開始直後の初期視点がワープしないように1度更新して位置を確定させる
  gameCamera_->Update(*services_.input);

  // プレイヤー初期化
  auto* inputPtr = services_.input;
  auto* cameraPtr = gameCamera_.get();
  AbsoluteEngine::ComponentFactory::GetInstance().Register("PlayerComponent", [inputPtr, cameraPtr]() {
      auto comp = std::make_unique<PlayerComponent>();
      comp->Initialize();
      comp->SetInput(inputPtr);
      comp->SetCamera(cameraPtr);
      return comp;
  });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("BulletComponent", []() { return std::make_unique<BulletComponent>(); });

  playerObj_ = std::make_shared<AbsoluteEngine::GameObject>("Player");
  playerObj_->LoadModel("resources/app/player/player.obj");
  auto playerComp = std::make_unique<PlayerComponent>();
  playerComp->Initialize();
  playerComp->SetInput(services_.input);
  playerComp->SetCamera(gameCamera_.get());
  playerObj_->AddComponent(std::move(playerComp));
  rootObjects_.push_back(playerObj_);

  // 敵の固定配置は削除されました（エディタでの配置に移行）

  auto* dx = Renderer::GetInstance()->GetDX();
  renderTexture_ = std::make_unique<RenderTexture>();
  renderTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {0.1f, 0.25f, 0.5f, 1.0f});

  depthTexture_ = std::make_unique<DepthTexture>();
  depthTexture_->Initialize(dx, 1280, 720);

  postProcessTexture_ = std::make_unique<RenderTexture>();
  postProcessTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {0.1f, 0.25f, 0.5f, 1.0f});

  gaussianTempTexture_ = std::make_unique<RenderTexture>();
  gaussianTempTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {0.1f, 0.25f, 0.5f, 1.0f});

  // サンプルコンポーネントの登録
  AbsoluteEngine::ComponentFactory::GetInstance().Register("SpinComponent", []() { return std::make_unique<SpinComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("MoveComponent", []() { return std::make_unique<MoveComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("StraightMoveComponent", []() { return std::make_unique<StraightMoveComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("EnemyComponent", []() { return std::make_unique<EnemyComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("EnemyShootComponent", []() { return std::make_unique<EnemyShootComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("BossComponent", []() { return std::make_unique<BossComponent>(); });

  // デフォルトのライトを一つ配置しておく
  auto initialDirLight = std::make_shared<AbsoluteEngine::GameObject>("Directional Light");
  initialDirLight->GetTransform().rotate = { 0.5f, 0.5f, 0.0f };
  initialDirLight->GetLight().type = AbsoluteEngine::LightComponent::Type::Directional;
  initialDirLight->GetLight().color = { 1.0f, 1.0f, 1.0f };
  initialDirLight->GetLight().intensity = 1.0f;
  rootObjects_.push_back(initialDirLight);
}

void GameScene::Finalize() {
}

void GameScene::SpawnHitEffect(const Vector3 &pos) {
  HitEffect ef;
  ef.instance.Initialize({ resEffect_, {1, 1, 1, 1}, 0 });
  ef.position = pos;
  ef.frame = 0.0f;
  ef.isActive = true;
  hitEffects_.push_back(std::move(ef));

  // パーティクルも少し出す
  for (int i = 0; i < 10; ++i) {
      ParticleManager::GetInstance()->Emit(
          "default", pos, Vector3{0, 0, 0}, Vector3{0.1f, 1.0f, 1.0f},
          Vector3{0, 0, (float)i}, 0.5f, Vector4{1, 0.5f, 0, 1});
  }
}

void GameScene::Update() {
  bool isTransitioning = false;
  const float deltaTime = 1.0f / 60.0f;
  time_ += deltaTime;

  // プレイヤーを探す（ロード時などでポインタが切り替わった場合に対応）
  playerObj_.reset();
  for (const auto& obj : rootObjects_) {
      if (!obj) continue;
      
      bool isPlayer = (obj->GetName() == "Player" || obj->GetName() == "player");
      if (!isPlayer) {
          for (const auto& comp : obj->GetComponents()) {
              if (comp->GetTypeName() == "PlayerComponent") {
                  isPlayer = true;
                  break;
              }
          }
      }
      
      if (isPlayer) {
          playerObj_ = obj;
          break;
      }
  }

  UpdateEditor();

  if (playMode_ == PlayMode::Play) {
#ifdef USE_IMGUI
      bool isDragging = ImGui::GetDragDropPayload() != nullptr;
#else
      bool isDragging = false;
#endif

      if (isDebugCamera_) {
          if (!isDragging) debugCamera_->Update(*services_.input);
      } else {
          // ゲーム（レール）カメラ進行 (GameCamera 内にセットしたコントローラーを正しく動作させる)
          CameraContext ctx{};
          ctx.deltaTime = deltaTime;
          gameCamera_->SetContext(ctx);
          if (!isDragging && phase_ == GamePhase::InProgress) gameCamera_->Update(*services_.input);
      }

  // プレイヤー更新 (常にゲームカメラを基準とする)
  // PlayerComponentのUpdateは、rootObjects_の中にいるためUpdateEditor経由で自動的に呼ばれます。
  // ただしPlayMode時のみ。

  // ボスフェーズ移行
  if (phase_ == GamePhase::InProgress && railController_->GetProgress() >= 1.0f) {
      phase_ = GamePhase::Boss;
      auto bossObj = std::make_shared<AbsoluteEngine::GameObject>("Boss");
      Vector3 eye = gameCamera_->GetEye();
      Vector3 forward = gameCamera_->GetForward();
      bossObj->GetTransform().translate = { eye.x + forward.x * 20.0f, eye.y + forward.y * 20.0f, eye.z + forward.z * 20.0f };
      bossObj->GetTransform().scale = {3.0f, 3.0f, 3.0f};
      bossObj->LoadModel("resources/app/cube/cube.obj");
      bossObj->SetEnvironmentCoefficient(1.0f);
      bossObj->AddComponent(std::make_unique<BossComponent>());
      bossObj->AddComponent(std::make_unique<EnemyShootComponent>());
      rootObjects_.push_back(bossObj);
  }

  // 弾の発射（スペースキー）
  if (shootCooldown_ > 0.0f) shootCooldown_ -= deltaTime;
  if (services_.input->PressKey(DIK_SPACE) && shootCooldown_ <= 0.0f && resBullet_ && playerObj_) {
      shootCooldown_ = 0.25f; // 連射速度を適正化（光線化を防ぎ1発ずつの独立感を強調）
      auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("Bullet");
      bulletObj->LoadModel("resources/app/bullet/bullet.obj");
      bulletObj->GetTransform().translate = playerObj_->GetTransform().translate;
      Vector3 forward = gameCamera_->GetForward();
      Vector3 vel = { forward.x * 35.0f, forward.y * 35.0f, forward.z * 35.0f }; // 弾速を適正化
      auto bulletComp = std::make_unique<BulletComponent>();
      bulletComp->Initialize(vel);
      bulletObj->AddComponent(std::move(bulletComp));
      bulletObjs_.push_back(bulletObj);
      rootObjects_.push_back(bulletObj);
  }

  // 弾の更新（コンポーネント化されたためrootObjects_経由で呼ばれるが、削除管理はこちらで行う）
  // 消えた弾を削除
  bulletObjs_.erase(std::remove_if(bulletObjs_.begin(), bulletObjs_.end(), [&](const std::shared_ptr<AbsoluteEngine::GameObject>& obj) {
      if (!obj) return true;
      bool isActive = false;
      for (const auto& comp : obj->GetComponents()) {
          if (comp->GetTypeName() == "BulletComponent") {
              auto* bComp = dynamic_cast<BulletComponent*>(comp.get());
              if (bComp) isActive = bComp->IsActive();
              break;
          }
      }
      if (!isActive) {
          rootObjects_.erase(std::remove(rootObjects_.begin(), rootObjects_.end(), obj), rootObjects_.end());
          return true;
      }
      return false;
  }), bulletObjs_.end());

  // --- 敵からの弾の発射処理 ---
  for (auto& obj : rootObjects_) {
      if (!obj) continue;
      
      for (const auto& comp : obj->GetComponents()) {
          if (comp->GetTypeName() == "EnemyShootComponent") {
              auto* shootComp = dynamic_cast<EnemyShootComponent*>(comp.get());
              if (shootComp && shootComp->WantToShoot() && playerObj_) {
                  Vector3 spawnPos = obj->GetTransform().translate;
                  Vector3 targetPos = playerObj_->GetTransform().translate;
                  
                  Vector3 diff = { targetPos.x - spawnPos.x, targetPos.y - spawnPos.y, targetPos.z - spawnPos.z };
                  Vector3 dir = Normalize(diff);
                  Vector3 vel = { dir.x * 20.0f, dir.y * 20.0f, dir.z * 20.0f }; // 弾速
                  
                  auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("EnemyBullet");
                  bulletObj->LoadModel("resources/app/bullet/bullet.obj");
                  bulletObj->GetTransform().translate = spawnPos;
                  auto bulletComp = std::make_unique<BulletComponent>();
                  bulletComp->Initialize(vel);
                  bulletObj->AddComponent(std::move(bulletComp));
                  enemyBulletObjs_.push_back(bulletObj);
                  rootObjects_.push_back(bulletObj);
                  
                  shootComp->ClearShootFlag();
              }
              break;
          }
      }
  }

  // 敵の弾の更新（削除管理）
  enemyBulletObjs_.erase(std::remove_if(enemyBulletObjs_.begin(), enemyBulletObjs_.end(), [&](const std::shared_ptr<AbsoluteEngine::GameObject>& obj) {
      if (!obj) return true;
      bool isActive = false;
      for (const auto& comp : obj->GetComponents()) {
          if (comp->GetTypeName() == "BulletComponent") {
              auto* bComp = dynamic_cast<BulletComponent*>(comp.get());
              if (bComp) isActive = bComp->IsActive();
              break;
          }
      }
      if (!isActive) {
          rootObjects_.erase(std::remove(rootObjects_.begin(), rootObjects_.end(), obj), rootObjects_.end());
          return true;
      }
      return false;
  }), enemyBulletObjs_.end());

  // --- 当たり判定（弾 vs 敵 GameObject） ---
  for (auto& bulletObj : bulletObjs_) {
      if (!bulletObj) continue;
      BulletComponent* bComp = nullptr;
      for (const auto& comp : bulletObj->GetComponents()) {
          if (comp->GetTypeName() == "BulletComponent") {
              bComp = dynamic_cast<BulletComponent*>(comp.get());
              break;
          }
      }
      if (!bComp || !bComp->IsActive()) continue;

      for (auto& obj : rootObjects_) {
          if (!obj || obj == bulletObj || obj == playerObj_) continue;
          
          EnemyComponent* enemyComp = nullptr;
          BossComponent* bossComp = nullptr;
          for (const auto& comp : obj->GetComponents()) {
              if (comp->GetTypeName() == "EnemyComponent") {
                  enemyComp = dynamic_cast<EnemyComponent*>(comp.get());
              } else if (comp->GetTypeName() == "BossComponent") {
                  bossComp = dynamic_cast<BossComponent*>(comp.get());
              }
          }

          bool isHit = false;
          if (enemyComp && enemyComp->IsActive()) {
              Vector3 objPos = obj->GetTransform().translate;
              Vector3 bulletPos = bulletObj->GetTransform().translate;
              Vector3 diff = { bulletPos.x - objPos.x, bulletPos.y - objPos.y, bulletPos.z - objPos.z };
              float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
              float rSum = bComp->GetCollisionRadius() + enemyComp->GetCollisionRadius();
              if (distSq <= rSum * rSum) {
                  SpawnHitEffect(objPos);
                  enemyComp->OnHit();
                  isHit = true;
              }
          } else if (bossComp && bossComp->IsActive()) {
              Vector3 objPos = obj->GetTransform().translate;
              Vector3 bulletPos = bulletObj->GetTransform().translate;
              Vector3 diff = { bulletPos.x - objPos.x, bulletPos.y - objPos.y, bulletPos.z - objPos.z };
              float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
              float rSum = bComp->GetCollisionRadius() + bossComp->GetCollisionRadius();
              if (distSq <= rSum * rSum) {
                  SpawnHitEffect(objPos);
                  bossComp->TakeDamage(1);
                  isHit = true;
                  if (!bossComp->IsActive()) {
                      phase_ = GamePhase::Clear;
                      RequestSceneChange(SceneId::Clear);
                      isTransitioning = true;
                  }
              }
          }

          if (isHit) {
              bComp->Deactivate();
              break;
          }
      }
  }

  // 撃破された（IsActive() == false）EnemyComponentまたはBossComponentを持つGameObjectをシーンから削除
  rootObjects_.erase(std::remove_if(rootObjects_.begin(), rootObjects_.end(), [](const std::shared_ptr<AbsoluteEngine::GameObject>& obj) {
      if (!obj) return true;
      for (const auto& comp : obj->GetComponents()) {
          if (comp->GetTypeName() == "EnemyComponent") {
              auto* enemyComp = dynamic_cast<EnemyComponent*>(comp.get());
              if (enemyComp && !enemyComp->IsActive()) return true;
          } else if (comp->GetTypeName() == "BossComponent") {
              auto* bossComp = dynamic_cast<BossComponent*>(comp.get());
              if (bossComp && !bossComp->IsActive()) return true;
          }
      }
      return false;
  }), rootObjects_.end());

  // --- 当たり判定（敵の弾 vs プレイヤー） ---
  if (playerObj_) {
      PlayerComponent* pComp = nullptr;
      for (const auto& comp : playerObj_->GetComponents()) {
          if (comp->GetTypeName() == "PlayerComponent") {
              pComp = dynamic_cast<PlayerComponent*>(comp.get());
              break;
          }
      }

      if (pComp) {
          for (auto& bulletObj : enemyBulletObjs_) {
              if (!bulletObj) continue;
              BulletComponent* bComp = nullptr;
              for (const auto& comp : bulletObj->GetComponents()) {
                  if (comp->GetTypeName() == "BulletComponent") {
                      bComp = dynamic_cast<BulletComponent*>(comp.get());
                      break;
                  }
              }
              if (!bComp || !bComp->IsActive()) continue;

              Vector3 bulletPos = bulletObj->GetTransform().translate;
              Vector3 playerPos = playerObj_->GetTransform().translate;
              
              Vector3 diff = {
                  bulletPos.x - playerPos.x,
                  bulletPos.y - playerPos.y,
                  bulletPos.z - playerPos.z
              };
              float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
              
              float playerRadius = 1.0f; // プレイヤーの当たり判定（仮）
              float rSum = bComp->GetCollisionRadius() + playerRadius;

              if (distSq <= rSum * rSum) {
                  // 被弾！
                  SpawnHitEffect(playerPos);
                  bComp->Deactivate();
                  pComp->TakeDamage(1);
                  if (pComp->IsDead()) {
                      phase_ = GamePhase::GameOver;
                      RequestSceneChange(SceneId::GameOver);
                      isTransitioning = true;
                  }
              }
          }
      }
  }

  // ヒットエフェクトの更新
  for (auto& ef : hitEffects_) {
      if (!ef.isActive) continue;
      ef.frame += 1.0f;
      float t = ef.frame / ef.maxFrame;
      float scaleVal = t * 4.0f;
      ef.instance.SetWorld(MakeAffineMatrix(Vector3{scaleVal, scaleVal, scaleVal}, Vector3{0.0f, 0.0f, 0.0f}, ef.position));
      ef.instance.SetColor({1.0f, 0.5f, 0.0f, 1.0f - t});
      if (ef.frame >= ef.maxFrame) ef.isActive = false;
  }
  hitEffects_.erase(std::remove_if(hitEffects_.begin(), hitEffects_.end(), [](const HitEffect& e) { return !e.isActive; }), hitEffects_.end());
  } // end of if (playMode_ == PlayMode::Play)

#ifdef USE_IMGUI
  DrawEditorUI();

  // --- ゲームビューポートウィンドウ ---
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Viewport##GameView");
  ImGui::PopStyleVar();
  if (!isTransitioning && postProcessTexture_) {
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
    if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = postProcessTexture_->GetSrvGpuHandle();
    ImGui::Image(static_cast<ImTextureID>(srvHandle.ptr), viewportSize);
    
    if (editorUIManager_) {
        Matrix4x4 viewMat, projMat;
        if (playMode_ == PlayMode::Edit && editorCamera_) {
            viewMat = editorCamera_->GetViewMatrix();
            projMat = editorCamera_->GetProjectionMatrix();
        } else if (isDebugCamera_ && debugCamera_) {
            viewMat = debugCamera_->GetViewMatrix();
            projMat = debugCamera_->GetProjectionMatrix();
        } else if (gameCamera_) {
            viewMat = gameCamera_->GetViewMatrix();
            projMat = gameCamera_->GetProjectionMatrix();
        } else {
            viewMat = Renderer::GetInstance()->GetViewMatrix();
            projMat = Renderer::GetInstance()->GetProjectionMatrix();
        }
        editorUIManager_->HandleViewportDragDrop(rootObjects_, viewMat, projMat);
    }
  }
  ImGui::End();

  ImGui::Begin("GameScene Controls##LeftPanel");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Separator();
  
  ImGui::SeparatorText("Bullet Controls & Info");
  ImGui::Text("Active Bullets: %d", (int)bulletObjs_.size());
  if (!bulletObjs_.empty() && bulletObjs_[0]) {
      const auto& pos = bulletObjs_[0]->GetTransform().translate;
      ImGui::Text("Bullet[0] Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
  }
  
  int hp = 0;
  int maxHp = 0;
  if (playerObj_) {
      for (const auto& comp : playerObj_->GetComponents()) {
          if (comp->GetTypeName() == "PlayerComponent") {
              if (auto* pComp = dynamic_cast<PlayerComponent*>(comp.get())) {
                  hp = pComp->GetHp();
                  maxHp = pComp->GetMaxHp();
              }
          }
      }
  }
  ImGui::Text("Player HP: %d / %d", hp, maxHp);
  
  ImGui::SeparatorText("Camera Controls");
  Vector3 eye = gameCamera_->GetEye();
  Vector3 target = gameCamera_->GetTarget();
  ImGui::Text("Camera Eye: (%.2f, %.2f, %.2f)", eye.x, eye.y, eye.z);
  ImGui::Text("Camera Target: (%.2f, %.2f, %.2f)", target.x, target.y, target.z);
  ImGui::Text("Rail Progress: %.1f %%", railController_->GetProgress() * 100.0f);
  
  ImGui::Checkbox("Debug Camera Mode", &isDebugCamera_);
  if (isDebugCamera_) {
      Vector3 camPos = debugCamera_->GetTranslate();
      if (ImGui::DragFloat3("Debug Camera Pos", &camPos.x, 0.1f)) {
          debugCamera_->SetTranslate(camPos);
      }
      Vector3 camRot = debugCamera_->GetRotation();
      if (ImGui::DragFloat3("Debug Camera Rot", &camRot.x, 0.05f)) {
          debugCamera_->SetRotation(camRot);
      }
  }
  ImGui::Checkbox("Show Rail Debug Line", &showDebugRail_);
  if (ImGui::Button("Reset Rail Camera")) {
      railController_->ResetProgress();
      CameraContext ctx{ 1.0f / 60.0f };
      gameCamera_->SetContext(ctx);
      gameCamera_->Update(*services_.input);
  }

  ImGui::SeparatorText("Post Process");
  int mode = static_cast<int>(postProcessMode_);
  if (ImGui::RadioButton("Normal", &mode, static_cast<int>(Renderer::PostProcessMode::Normal))) postProcessMode_ = Renderer::PostProcessMode::Normal;
  ImGui::SameLine();
  if (ImGui::RadioButton("Grayscale", &mode, static_cast<int>(Renderer::PostProcessMode::Grayscale))) postProcessMode_ = Renderer::PostProcessMode::Grayscale;
  ImGui::SameLine();
  if (ImGui::RadioButton("Sepia", &mode, static_cast<int>(Renderer::PostProcessMode::Sepia))) postProcessMode_ = Renderer::PostProcessMode::Sepia;
  ImGui::SameLine();
  if (ImGui::RadioButton("Vignette", &mode, static_cast<int>(Renderer::PostProcessMode::Vignette))) postProcessMode_ = Renderer::PostProcessMode::Vignette;
  ImGui::SameLine();
  if (ImGui::RadioButton("BoxFilter", &mode, static_cast<int>(Renderer::PostProcessMode::BoxFilter))) postProcessMode_ = Renderer::PostProcessMode::BoxFilter;
  ImGui::SameLine();
  if (ImGui::RadioButton("GaussianFilter", &mode, static_cast<int>(Renderer::PostProcessMode::GaussianFilter))) postProcessMode_ = Renderer::PostProcessMode::GaussianFilter;
  ImGui::SameLine();
  if (ImGui::RadioButton("LuminanceOutline", &mode, static_cast<int>(Renderer::PostProcessMode::LuminanceBasedOutline))) postProcessMode_ = Renderer::PostProcessMode::LuminanceBasedOutline;
  ImGui::SameLine();
  if (ImGui::RadioButton("DepthOutline", &mode, static_cast<int>(Renderer::PostProcessMode::DepthBasedOutline))) postProcessMode_ = Renderer::PostProcessMode::DepthBasedOutline;
  ImGui::SameLine();
  if (ImGui::RadioButton("Random", &mode, static_cast<int>(Renderer::PostProcessMode::Random))) postProcessMode_ = Renderer::PostProcessMode::Random;

  if (postProcessMode_ == Renderer::PostProcessMode::Vignette) {
      ImGui::SliderFloat("Vignette Scale", &vignetteScale_, 1.0f, 32.0f);
      ImGui::SliderFloat("Vignette Pow", &vignettePow_, 0.1f, 5.0f);
  } else if (postProcessMode_ == Renderer::PostProcessMode::BoxFilter) {
      ImGui::SliderInt("BoxFilter K", &boxFilterK_, 1, 10);
  } else if (postProcessMode_ == Renderer::PostProcessMode::GaussianFilter) {
      ImGui::SliderInt("GaussianFilter K", &gaussianFilterK_, 1, 10);
      ImGui::SliderFloat("GaussianFilter Sigma", &gaussianFilterSigma_, 0.1f, 10.0f);
  }

  ImGui::End();
#endif
}

void GameScene::Draw() {
  auto* renderer = Renderer::GetInstance();
  auto* dx = renderer->GetDX();

  if (renderTexture_ && depthTexture_) {
    dx->SetRenderTargetWithDepth(renderTexture_.get(), depthTexture_.get());
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    dx->GetCommandList()->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);

    dx->GetCommandList()->ClearDepthStencilView(depthTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  }

  if (playMode_ == PlayMode::Edit) {
      if (editorCamera_) renderer->SetCamera(*editorCamera_);
  } else if (isDebugCamera_) {
      renderer->SetCamera(*debugCamera_);
  } else {
      renderer->SetCamera(*gameCamera_);
  }
  renderer->SetEnvironmentMap(skybox_.GetTexture());
  renderer->SetVignetteParam(vignetteScale_, vignettePow_);
  renderer->SetBoxFilterParam(boxFilterK_);
  renderer->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {1.0f, 0.0f});
  renderer->SetRandomParam(time_);

  // --- ライトの適用 ---
  ApplyEditorLightsToRenderer(renderer);

  // オブジェクトの描画
  skybox_.Draw();

  // エディタ上で配置したオブジェクト群の描画
  // rootObjects_にすべて登録されているので、手動での描画は不要。
  for (auto& obj : rootObjects_) {
      if (obj) obj->Draw();
  }

  for (auto& ef : hitEffects_) {
      renderer->DrawEffectModel(&ef.instance);
  }

  // --- デバッグラインの描画 ---
  if (showDebugRail_) {
      const auto& waypoints = railController_->GetWaypoints();
      if (waypoints.size() >= 2) {
          Matrix4x4 viewMat = renderer->GetViewMatrix();
          Matrix4x4 invView = Inverse(viewMat);
          Vector3 camEye = { invView.m[3][0], invView.m[3][1], invView.m[3][2] };
          // カメラ自身のワールド空間における正確な正面ベクトル（invViewの第3行）
          Vector3 camForward = { invView.m[2][0], invView.m[2][1], invView.m[2][2] };

          Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f}; // 黄色
          const int subdivisions = 100;
          Vector3 prevPoint = Spline::GetPoint(waypoints, 0.0f);
          for (int i = 1; i <= subdivisions; ++i) {
              float t = (float)i / (float)subdivisions;
              Vector3 currPoint = Spline::GetPoint(waypoints, t);
              
              // カメラより前にあるポイント間だけを描画（距離の大小に影響されない厳格な前方カリング）
              Vector3 diff1 = { prevPoint.x - camEye.x, prevPoint.y - camEye.y, prevPoint.z - camEye.z };
              Vector3 diff2 = { currPoint.x - camEye.x, currPoint.y - camEye.y, currPoint.z - camEye.z };
              if (Dot(diff1, camForward) > 0.0f && Dot(diff2, camForward) > 0.0f) {
                  renderer->DrawLine(prevPoint, currPoint, color);
              }
              prevPoint = currPoint;
          }
          // ウェイポイントマーカーも同様に厳格に前方カリング
          for (const auto& wp : waypoints) {
              Vector3 diff = { wp.x - camEye.x, wp.y - camEye.y, wp.z - camEye.z };
              if (Dot(diff, camForward) > 0.0f) {
                  renderer->DrawLine(Vector3{wp.x, wp.y - 1.0f, wp.z}, Vector3{wp.x, wp.y + 1.0f, wp.z}, Vector4{1, 0, 0, 1});
                  renderer->DrawLine(Vector3{wp.x - 1.0f, wp.y, wp.z}, Vector3{wp.x + 1.0f, wp.y, wp.z}, Vector4{1, 0, 0, 1});
              }
          }
      }
  }

  // 進行速度と空間の奥行きを実感しやすくするため、地面に広大なグリッドを描画
  renderer->DrawGrid(500.0f, 50, Vector4{0.2f, 0.4f, 0.8f, 0.5f});

  renderer->RenderPrimitives();
  renderer->DrawGPUParticles();

  if (renderTexture_ && depthTexture_) {
      dx->FinishRenderingWithDepth(renderTexture_.get(), depthTexture_.get());
  }

  if (renderTexture_ && postProcessTexture_) {
    if (postProcessMode_ == Renderer::PostProcessMode::GaussianFilter && gaussianTempTexture_) {
      // パス1: 横方向
      dx->SetRenderTarget(gaussianTempTexture_.get());
      renderer->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {1.0f, 0.0f});
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(gaussianTempTexture_.get());

      // パス2: 縦方向
      dx->SetRenderTarget(postProcessTexture_.get());
      renderer->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {0.0f, 1.0f});
      renderer->DrawFullscreen(gaussianTempTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(postProcessTexture_.get());
    } else if (postProcessMode_ == Renderer::PostProcessMode::DepthBasedOutline) {
      dx->SetRenderTarget(postProcessTexture_.get());
      Matrix4x4 projInverse;
      if (playMode_ == PlayMode::Edit && editorCamera_) projInverse = Inverse(editorCamera_->GetProjectionMatrix());
      else if (isDebugCamera_) projInverse = Inverse(debugCamera_->GetProjectionMatrix());
      else projInverse = Inverse(gameCamera_->GetProjectionMatrix());
      renderer->SetDepthBasedOutlineParam(projInverse);
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_, depthTexture_->GetSrvGpuHandle());
      dx->FinishRendering(postProcessTexture_.get());
    } else {
      dx->SetRenderTarget(postProcessTexture_.get());
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(postProcessTexture_.get());
    }
  }
}