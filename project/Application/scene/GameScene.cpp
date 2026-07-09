#define NOMINMAX
#include "GameScene.h"
#include "Renderer.h"
#include "DirectXCommon.h"
#include "DirectXResourceUtils.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Input.h"
#include "../camera/RailCameraComponent.h"
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
#include "../actor/Enemy/StraightMoveComponent.h"
#include "../actor/Enemy/EnemyComponent.h"
#include "../actor/Enemy/EnemyShootComponent.h"
#include "../actor/Enemy/BossComponent.h"
#include "SceneIds.h"
#include "AbsoluteEngine/resources/AssetManager.h"
#include "AbsoluteEngine/scene/SceneSerializer.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/LightNodeComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  auto* am = AbsoluteEngine::AssetManager::GetInstance();

  // リソースのロード
  resPlayer_ = am->Load<ModelResource>("resources/app/player/player.obj"); // 追加された自機モデル
  resBullet_ = am->Load<ModelResource>("resources/app/bullet/bullet.obj"); // 追加された弾モデル
  resEnemy_  = am->Load<ModelResource>("resources/app/cube/cube.obj"); // 敵モデル
  resEffect_ = am->Load<ModelResource>("resources/app/particle/particle.obj");

  if (!resBullet_) {
      // 万一 bullet.obj が読み込めない場合は、確実に存在する enemy (cube.obj) を仮割り当てする
      resBullet_ = resEnemy_;
  }

  skybox_.Initialize("resources/app/dds/dds.dds");

  // パーティクルグループの初期化（連射ヒット時のアサートクラッシュ防止）
  ParticleManager::GetInstance()->CreateParticleGroup("default", "resources/app/particle/circle.png", 500);

  // レールカメラの初期化
  auto camera = std::make_shared<GameCamera>();
  camera->Initialize();
  camera->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);
  SetMainCamera(camera);

  // デバッグカメラ初期化
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize();
  debugCamera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);

  AbsoluteEngine::ComponentFactory::GetInstance().Register("RailCameraComponent", []() {
      auto comp = std::make_unique<RailCameraComponent>();
      return comp;
  });
  // コンポーネントファクトリの登録
  AbsoluteEngine::ComponentFactory::GetInstance().Register("PlayerComponent", []() {
      auto comp = std::make_unique<PlayerComponent>();
      comp->Initialize();
      return comp;
  });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("BulletComponent", []() { return std::make_unique<BulletComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("StraightMoveComponent", []() { return std::make_unique<StraightMoveComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("EnemyComponent", []() { return std::make_unique<EnemyComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("EnemyShootComponent", []() { return std::make_unique<EnemyShootComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("BossComponent", []() { return std::make_unique<BossComponent>(); });

  // オートロード：保存されたシーンを読み込む
  LoadEditorScene();

  // シーン開始直後の初期視点がワープしないように1度更新して位置を確定させる
  GetMainCamera()->Update(*services_.input);

  // プレイヤーを探す（ロードされたデータにあるか）
  playerObj_.reset();
  for (const auto& obj : rootObjects_) {
      if (obj && (obj->GetName() == "Player" || obj->GetName() == "player")) {
          playerObj_ = obj;
          break;
      }
  }

  // プレイヤーがいなければ生成
  if (!playerObj_) {
      playerObj_ = std::make_shared<AbsoluteEngine::GameObject>("Player");
      auto playerModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
      playerModelComp->LoadModel("resources/app/player/player.obj");
      playerObj_->AddComponent(std::move(playerModelComp));
      rootObjects_.push_back(playerObj_);
  }
  
  // PlayerComponent がアタッチされているか確認し、無ければ追加・初期化
  auto playerComp = playerObj_->GetComponent<PlayerComponent>();
  if (!playerComp) {
      auto pComp = std::make_unique<PlayerComponent>();
      pComp->Initialize();
      playerObj_->AddComponent(std::move(pComp));
  } else {
      playerComp->Initialize();
  }

  Renderer::GetInstance()->InitializePostProcess(1280, 720);

  // デフォルトのライトを探す
  bool hasLight = false;
  for (const auto& obj : rootObjects_) {
      if (obj && obj->GetComponent<AbsoluteEngine::LightNodeComponent>()) {
          hasLight = true;
          break;
      }
  }
  if (!hasLight) {
      auto initialDirLight = std::make_shared<AbsoluteEngine::GameObject>("Directional Light");
      initialDirLight->GetTransform().rotate = { 0.5f, 0.5f, 0.0f };
      auto lightComp = std::make_unique<AbsoluteEngine::LightNodeComponent>();
      lightComp->type = AbsoluteEngine::LightNodeComponent::Type::Directional;
      lightComp->color = { 1.0f, 1.0f, 1.0f };
      lightComp->intensity = 1.0f;
      initialDirLight->AddComponent(std::move(lightComp));
      rootObjects_.push_back(initialDirLight);
  }

  // レールカメラを探す
  bool hasRailCamera = false;
  for (const auto& obj : rootObjects_) {
      if (obj && obj->GetComponent<RailCameraComponent>()) {
          hasRailCamera = true;
          break;
      }
  }
  if (!hasRailCamera) {
      auto railCamObj = std::make_shared<AbsoluteEngine::GameObject>("RailCamera");
      auto rComp = std::make_unique<RailCameraComponent>();
      railCamObj->AddComponent(std::move(rComp));
      rootObjects_.push_back(railCamObj);
  }

  // HUDの初期化
  gameHUD_ = std::make_unique<GameHUD>();
  gameHUD_->Initialize();
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
          GetMainCamera()->SetContext(ctx);
          if (!isDragging && phase_ == GamePhase::InProgress) GetMainCamera()->Update(*services_.input);
      }

  // プレイヤー更新 (常にゲームカメラを基準とする)
  // PlayerComponentのUpdateは、rootObjects_の中にいるためUpdateEditor経由で自動的に呼ばれます。
  // ただしPlayMode時のみ。

  // ボスフェーズ移行
  float railProgress = 0.0f;
  for (const auto& obj : rootObjects_) {
      if (auto comp = obj->GetComponent<RailCameraComponent>()) {
          railProgress = comp->GetProgress();
          break;
      }
  }

  if (phase_ == GamePhase::InProgress && railProgress >= 1.0f) {
      phase_ = GamePhase::Boss;
      auto bossObj = std::make_shared<AbsoluteEngine::GameObject>("Boss");
      Vector3 eye = GetMainCamera()->GetEye();
      Vector3 forward = GetMainCamera()->GetForward();
      bossObj->GetTransform().translate = { eye.x + forward.x * 20.0f, eye.y + forward.y * 20.0f, eye.z + forward.z * 20.0f };
      bossObj->GetTransform().scale = {3.0f, 3.0f, 3.0f};
      auto bossModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
      bossModelComp->LoadModel("resources/app/cube/cube.obj");
      bossModelComp->SetEnvironmentCoefficient(1.0f);
      bossObj->AddComponent(std::move(bossModelComp));
      bossObj->AddComponent(std::make_unique<BossComponent>());
      bossObj->AddComponent(std::make_unique<EnemyShootComponent>());

      auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
      colliderComp->type = AbsoluteEngine::ColliderComponent::Type::Sphere;
      colliderComp->radius = 5.0f; // ボスなので大きめに
      bossObj->AddComponent(std::move(colliderComp));
      bossObj->SetTag("Boss");

      rootObjects_.push_back(bossObj);
  }

  // --- プレイヤーとボスの生存チェック（シーン遷移用） ---
  if (playerObj_) {
      auto pComp = playerObj_->GetComponent<PlayerComponent>();
      if (pComp && pComp->IsDead()) {
          phase_ = GamePhase::GameOver;
          RequestSceneChange(SceneId::GameOver);
          isTransitioning = true;
      }
  }

  if (phase_ == GamePhase::Boss) {
      bool bossAlive = false;
      for (auto& obj : rootObjects_) {
          if (obj && obj->GetComponent<BossComponent>()) {
              bossAlive = true;
              break;
          }
      }
      if (!bossAlive) {
          phase_ = GamePhase::Clear;
          RequestSceneChange(SceneId::Clear);
          isTransitioning = true;
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
  
  // HUDの更新
  if (gameHUD_) {
      int hp = 0;
      int maxHp = 0;
      std::vector<Vector2> lockPositions;
      bool isLockingMode = false;

      if (playerObj_) {
          if (auto pComp = playerObj_->GetComponent<PlayerComponent>()) {
              hp = pComp->GetHp();
              maxHp = pComp->GetMaxHp();
              
              // ロックオン情報の取得
              isLockingMode = pComp->lockon_.IsLockingMode();
              lockPositions = pComp->lockon_.GetLockedScreenPositions(GetMainCamera());
              
              gameHUD_->Update(hp, maxHp, lockPositions, isLockingMode, pComp->GetCursorPos());
          }
      } else {
          gameHUD_->Update(hp, maxHp, lockPositions, isLockingMode, {640.0f, 360.0f});
      }
  }

  } // end of if (playMode_ == PlayMode::Play)


  // シーンの自動セーブ
  bool shouldSave = false;
  for (const auto& obj : rootObjects_) {
      if (auto rComp = obj->GetComponent<RailCameraComponent>()) {
          if (rComp->ConsumeModifiedFlag()) {
              shouldSave = true;
          }
      }
  }
  if (shouldSave) {
      SaveEditorScene();
  }
}


void GameScene::Draw() {
  auto *renderer = Renderer::GetInstance();
  renderer->BeginRenderScene();

  if (playMode_ == PlayMode::Edit) {
      if (editorCamera_) renderer->SetCamera(*editorCamera_);
  } else if (isDebugCamera_) {
      renderer->SetCamera(*debugCamera_);
  } else {
      renderer->SetCamera(*GetMainCamera());
  }
  renderer->SetEnvironmentMap(skybox_.GetTexture());
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
      std::vector<Vector3> waypoints;
      for (const auto& obj : rootObjects_) {
          if (auto rComp = obj->GetComponent<RailCameraComponent>()) {
              waypoints = rComp->GetWaypoints();
              break;
          }
      }
      
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
  // renderer->DrawGPUParticles(); // 実装確認用のパーティクルを描画・更新しないようにコメントアウト

  // HUD等の2Dスプライト描画
  if (playMode_ == PlayMode::Play && gameHUD_) {
      gameHUD_->Draw();
  }

  Matrix4x4 projInverse;
  if (playMode_ == PlayMode::Edit && editorCamera_) projInverse = Inverse(editorCamera_->GetProjectionMatrix());
  else if (isDebugCamera_) projInverse = Inverse(debugCamera_->GetProjectionMatrix());
  else projInverse = Inverse(GetMainCamera()->GetProjectionMatrix());

  renderer->EndRenderScene(projInverse);
}



void GameScene::DrawEditorUI() {
    BaseScene::DrawEditorUI(); // ツールバー等の描画

#ifdef USE_IMGUI
    if (phase_ != GamePhase::GameOver) {
        // [REMOVED] Duplicated ImGui::Begin("Viewport##GameView") which breaks ImGui rendering
    }

    ImGui::Begin("GameScene Controls##LeftPanel");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    
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
    Vector3 eye = GetMainCamera()->GetEye();
    Vector3 target = GetMainCamera()->GetTarget();
    ImGui::Text("Camera Eye: (%.2f, %.2f, %.2f)", eye.x, eye.y, eye.z);
    ImGui::Text("Camera Target: (%.2f, %.2f, %.2f)", target.x, target.y, target.z);
    
    if (showDebugRail_) {
        // Find component
        RailCameraComponent* rComp = nullptr;
        for (const auto& obj : rootObjects_) {
            if (auto comp = obj->GetComponent<RailCameraComponent>()) {
                rComp = comp;
                break;
            }
        }
        if (rComp) {
            ImGui::Text("Rail Progress: %.1f %%", rComp->GetProgress() * 100.0f);
        }
    }

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
        for (const auto& obj : rootObjects_) {
            if (auto rComp = obj->GetComponent<RailCameraComponent>()) {
                rComp->ResetProgress();
                break;
            }
        }
        CameraContext ctx{ 1.0f / 60.0f };
        GetMainCamera()->SetContext(ctx);
        if (services_.input) GetMainCamera()->Update(*services_.input);
    }

    ImGui::End();
#endif
}