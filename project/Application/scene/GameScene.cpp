#define NOMINMAX
#include "GameScene.h"
#include "Renderer.h"
#include "DirectXCommon.h"
#include "DirectXResourceUtils.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "particle/GPUParticleManager.h"
#include "Input.h"
#include "../camera/RailCameraComponent.h"
#include "Spline.h"
#include "Method.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>  // rand()

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
#include "AnimationManager.h"
#include "AbsoluteEngine/scene/LightNodeComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"
#include "AbsoluteEngine/scene/DissolveComponent.h"
#include "graphics/particle/EffectManager.h"
#include "graphics/particle/RingEffect.h"
#include "component/ExplosionLightComponent.h"

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  auto* am = AbsoluteEngine::AssetManager::GetInstance();

  // リソースのロード
  resPlayer_ = am->Load<ModelResource>("resources/app/player/player.obj");
  resBullet_ = am->Load<ModelResource>("resources/app/bullet/bullet.obj");
  resEnemy_  = am->Load<ModelResource>("resources/app/cube/cube.obj"); // 敵モデル
  resEffect_ = am->Load<ModelResource>("resources/app/particle/particle.obj");

  // ヒットエフェクト用テクスチャのロード
  texRing_   = am->Load<TextureResource>("resources/app/textures/gradationLine.png");
  texNoise0_ = am->Load<TextureResource>("resources/noise/noise0.png");
  // ディゾルブのマスクテクスチャをレンダラーにセット
  if (texNoise0_) {
      Renderer::GetInstance()->SetDissolveMaskTexture(texNoise0_);
  }

  if (!resBullet_) {
      // 万一 bullet.obj が読み込めない場合は、確実に存在する enemy (cube.obj) を仮割り当てする
      resBullet_ = resEnemy_;
  }

  skybox_.Initialize("resources/app/dds/dds.dds");

  // パーティクルグループの初期化（連射ヒット時のアサートクラッシュ防止）
  ParticleManager::GetInstance()->CreateParticleGroup("default", "resources/app/particle/circle.png", 500);
  ParticleManager::GetInstance()->CreateParticleGroup("explosion", "resources/explosion/explosion.png", 100);

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

  // -----------------------------------------------------------------------
  // 【バグ修正】エディタで配置した敵への onDestroyed コールバック一括登録
  // SpawnManager 経由で生成した敵は Spawn 時にコールバックを登録するが、
  // JSON/エディタからロードした敵にはその処理が走らないため、ここで補完する。
  // -----------------------------------------------------------------------
  for (const auto& obj : rootObjects_) {
      if (!obj) continue;
      auto* enemyComp = obj->GetComponent<EnemyComponent>();
      if (!enemyComp) continue;
      // 既にコールバックが登録済みの場合は上書きしない
      if (enemyComp->onDestroyed) continue;

      enemyComp->onDestroyed = [this](const Vector3& hitPos) {
          // 1. ヒットエフェクト（パーティクル・リングエフェクト・爆発ライト）
          SpawnHitEffect(hitPos);

          // 2. カメラシェイク開始
          cameraShakeDuration_  = 0.3f;
          cameraShakeTimer_     = 0.0f;
          cameraShakeIntensity_ = 0.2f;

          // 3. 画面歪み（RadialBlur）開始
          hitDistortionDuration_  = 0.25f;
          hitDistortionTimer_     = 0.0f;
          hitDistortionIntensity_ = 0.06f;

          // 4. RadialBlur の中心を敵のスクリーン座標に設定
          auto* cam = isDebugCamera_
              ? static_cast<Camera*>(debugCamera_.get())
              : static_cast<Camera*>(GetMainCamera());
          if (cam) {
              Matrix4x4 vp = Multiply(cam->GetViewMatrix(), cam->GetProjectionMatrix());
              Vector3 sp = TransformPoint(hitPos, vp);
              radialBlurCenter_ = { sp.x * 0.5f + 0.5f, -sp.y * 0.5f + 0.5f };
          }
      };
  }

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

  // -----------------------------------------------------------------------
  // プレイヤーモデルを骨あり(Skinning対応)のgltfに強制的に差し替える。
  // resources/editor/scenes/Game.json 上は旧player.objのままでも、
  // 実行時に必ずSkinning/Animation機能が可視化されるようにするための処置。
  // -----------------------------------------------------------------------
  {
      auto* playerModelComp = playerObj_->GetComponent<AbsoluteEngine::ModelComponent>();
      if (!playerModelComp) {
          auto newModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
          playerObj_->AddComponent(std::move(newModelComp));
          playerModelComp = playerObj_->GetComponent<AbsoluteEngine::ModelComponent>();
      }
      playerModelComp->LoadModel("resources/app/human/walk.gltf");

      if (!playerWalkAnim_) {
          playerWalkAnim_ = AnimationManager::GetInstance()->LoadAnimation("resources/app/human", "walk.gltf");
      }
      // ロックオンON/OFF時のAnimation補間（クロスフェード）実演用に別アニメーションもロードしておく
      if (!playerSneakWalkAnim_) {
          playerSneakWalkAnim_ = AnimationManager::GetInstance()->LoadAnimation("resources/app/human", "sneakWalk.gltf");
      }
      if (auto* instance = playerModelComp->GetModelInstance()) {
          instance->PlayAnimation(playerWalkAnim_, true);
      }
  }

  // -----------------------------------------------------------------------
  // GPU Particle拡張の常時可視化（加点要素）：
  // プレイヤー付近に常駐エミッタ(Box)を1つ設置し、Vortex Fieldで渦を巻かせる。
  // フレーム開始直後からゲーム画面で複数エミッタ/Fieldが確認できるようにする。
  // -----------------------------------------------------------------------
  {
      GPUParticleManager::EmitterDesc desc;
      desc.shape = GPUParticleManager::EmitterShape::Box;
      desc.translate = { 0.0f, 2.0f, 10.0f };
      desc.halfExtents = { 2.0f, 2.0f, 2.0f };
      desc.count = 12;
      desc.frequency = 0.3f;
      GPUParticleManager::GetInstance()->CreateEmitter(desc);

      GPUParticleManager::FieldDesc fieldDesc;
      fieldDesc.type = GPUParticleManager::FieldType::Vortex;
      fieldDesc.target = { 0.0f, 2.0f, 10.0f };
      fieldDesc.direction = { 0.0f, 1.0f, 0.0f };
      fieldDesc.strength = 1.5f;
      GPUParticleManager::GetInstance()->SetField(0, fieldDesc);
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

  // -----------------------------------------------------------------------
  // MultiMesh & MultiMaterial対応（加点要素）の常時可視化：
  // multiMaterial.objは複数メッシュ×複数マテリアルを持つアセットで、
  // Renderer::DrawModelのサブメッシュ描画パスをゲームシーン上で確認できるようにする。
  // -----------------------------------------------------------------------
  {
      // シーンJSONへのオートセーブで既に保存されている場合に毎回重複生成しないよう、
      // 名前で既存チェックしてから生成する。
      bool hasMultiMaterialDecoration = false;
      for (const auto& obj : rootObjects_) {
          if (obj && obj->GetName() == "MultiMaterialDecoration") {
              hasMultiMaterialDecoration = true;
              break;
          }
      }
      if (!hasMultiMaterialDecoration) {
          auto multiMaterialObj = std::make_shared<AbsoluteEngine::GameObject>("MultiMaterialDecoration");
          multiMaterialObj->GetTransform().translate = { 8.0f, 1.0f, 5.0f };
          multiMaterialObj->GetTransform().scale = { 2.0f, 2.0f, 2.0f };
          auto multiMaterialModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
          multiMaterialModelComp->LoadModel("resources/app/multiMaterial/multiMaterial.obj");
          multiMaterialObj->AddComponent(std::move(multiMaterialModelComp));
          rootObjects_.push_back(multiMaterialObj);
      }
  }

  // HUDの初期化
  gameHUD_ = std::make_unique<GameHUD>();
  gameHUD_->Initialize();

  // -----------------------------------------------------------------------
  // タイムラインマネージャーにRailCameraComponentを登録する
  // LoadEditorScene() 後に rootObjects_ を走査して RailCameraComponent を探し、
  // BaseScene 経由でタイムラインと紐付ける
  // -----------------------------------------------------------------------
  for (const auto& obj : rootObjects_) {
      if (!obj) continue;
      auto* railComp = obj->GetComponent<RailCameraComponent>();
      if (railComp) {
          SetTimelineRailCamera(railComp);
          break; // レールカメラは1つだけなので最初に見つかったものを使う
      }
  }
  // タイムラインデータを読み込む（ファイルが無ければ空の状態で開始）
  LoadTimeline();

  // -----------------------------------------------------------------------
  // SpawnManagerの初期化：デモ用スポーンイベントを登録する
  // -----------------------------------------------------------------------
  spawnTimer_ = 0.0f;
  //InitSpawnEvents_();

#ifndef USE_IMGUI
  // ImGuiが無いビルド（Release）ではPlayボタンを押す手段が無いため、
  // タイトルからゲームシーンに来た時点で自動的にPlayモードへ入る。
  BackupScene();
  SetPlayMode(PlayMode::Play);
  timelineManager_.Play();
#endif
}

// -----------------------------------------------------------------------
// InitSpawnEvents_
// 発表デモ用のウェーブデータをハードコードで定義する。
// 将来的にはJSON/タイムラインエディタから読み込む形に置き換える。
// -----------------------------------------------------------------------
//void GameScene::InitSpawnEvents_() {
//  spawnEvents_.clear();
//
//  // フォーマット: { triggerTime(秒), prefabId, position(X,Y,Z) }
//  // カメラのレールが前進するにつれて、前方に敵が出現する構成
//
//  // --- 第1波：ゲーム開始直後（3秒後）に正面へ3体出現 ---
//  spawnEvents_.push_back({ 3.0f,  "Enemy", { -6.0f, 0.0f,  60.0f } });
//  spawnEvents_.push_back({ 3.0f,  "Enemy", {  0.0f, 2.0f,  65.0f } });
//  spawnEvents_.push_back({ 3.0f,  "Enemy", {  6.0f, 0.0f,  60.0f } });
//
//  // --- 第2波：6秒後に斜め配置で2体出現（上下に散らして見栄えを出す） ---
//  spawnEvents_.push_back({ 6.0f,  "Enemy", { -8.0f,  3.0f, 90.0f } });
//  spawnEvents_.push_back({ 6.0f,  "Enemy", {  8.0f, -3.0f, 90.0f } });
//
//  // --- 第3波：10秒後に前方中央に密集した4体 ---
//  spawnEvents_.push_back({ 10.0f, "Enemy", { -4.0f,  2.0f, 120.0f } });
//  spawnEvents_.push_back({ 10.0f, "Enemy", {  4.0f,  2.0f, 120.0f } });
//  spawnEvents_.push_back({ 10.0f, "Enemy", { -4.0f, -2.0f, 125.0f } });
//  spawnEvents_.push_back({ 10.0f, "Enemy", {  4.0f, -2.0f, 125.0f } });
//
//  // --- 第4波：14秒後の最終波（大きく散らして迫力を演出） ---
//  spawnEvents_.push_back({ 14.0f, "Enemy", { -12.0f,  0.0f, 150.0f } });
//  spawnEvents_.push_back({ 14.0f, "Enemy", {   0.0f,  5.0f, 155.0f } });
//  spawnEvents_.push_back({ 14.0f, "Enemy", {  12.0f,  0.0f, 150.0f } });
//}

void GameScene::Finalize() {
}

// -----------------------------------------------------------------------
// GetEditorViewCamera オーバーライド
// Draw() のカメラ選択分岐（isDebugCamera_ / playMode_ による切り替え）と完全に一致させる。
// ここがズレると、Viewportに表示されている絵と、ギズモ・クリック判定の座標系が
// 食い違い、「実際のゲームカメラとは関係ない視点」に見えるバグになる。
// -----------------------------------------------------------------------
Camera* GameScene::GetEditorViewCamera() const {
    if (isDebugCamera_) {
        if (playMode_ == PlayMode::Edit && editorCamera_) {
            return editorCamera_.get();
        }
        return debugCamera_.get();
    }
    return GetMainCamera();
}

// -----------------------------------------------------------------------
// AddRootObject オーバーライド（タスクC: 依存性注入）
// BaseScene::AddRootObject を呼んだ後、追加オブジェクトが EnemyComponent を持って
// いれば onDestroyed コールバックを自動注入する。
// タイムライン経由でスポーンされた敵にもコールバックが登録される。
// -----------------------------------------------------------------------
void GameScene::AddRootObject(std::shared_ptr<AbsoluteEngine::GameObject> obj) {
    // まず基底クラスの処理でリストに追加する
    BaseScene::AddRootObject(obj);

    if (!obj) return;

    // EnemyComponent を持っているか確認する
    auto* enemyComp = obj->GetComponent<EnemyComponent>();
    if (!enemyComp) return;

    // 既にコールバックが登録済みの場合は上書きしない（二重登録防止）
    if (enemyComp->onDestroyed) return;

    // GameScene 固有の撃破コールバックを注入する
    enemyComp->onDestroyed = [this](const Vector3& hitPos) {
        // 1. ヒットエフェクト（パーティクル・リングエフェクト・爆発ライト）
        SpawnHitEffect(hitPos);

        // 2. カメラシェイク開始
        cameraShakeDuration_  = 0.3f;
        cameraShakeTimer_     = 0.0f;
        cameraShakeIntensity_ = 0.2f;

        // 3. 画面歪み（RadialBlur）開始
        hitDistortionDuration_  = 0.25f;
        hitDistortionTimer_     = 0.0f;
        hitDistortionIntensity_ = 0.06f;

        // 4. RadialBlur の中心を敵のスクリーン座標に設定
        auto* cam = isDebugCamera_
            ? static_cast<Camera*>(debugCamera_.get())
            : static_cast<Camera*>(GetMainCamera());
        if (cam) {
            Matrix4x4 vp = Multiply(cam->GetViewMatrix(), cam->GetProjectionMatrix());
            Vector3 sp = TransformPoint(hitPos, vp);
            radialBlurCenter_ = { sp.x * 0.5f + 0.5f, -sp.y * 0.5f + 0.5f };
        }
    };
}

// -----------------------------------------------------------------------
// zEffect
// DevScene の実装を GameScene に移植。
// パーティクル + RingEffect + ExplosionLight の3段構えのエフェクト。
// -----------------------------------------------------------------------
void GameScene::SpawnHitEffect(const Vector3 &pos) {
  // 1. 爆発パーティクル（explosion グループ）を散布
  for (int i = 0; i < 30; ++i) {
      float rX = ((rand() % 200) / 100.0f) - 1.0f;
      float rY = ((rand() % 200) / 100.0f) - 1.0f;
      float rZ = ((rand() % 200) / 100.0f) - 1.0f;
      Vector3 vel = { rX * 1.5f, rY * 1.5f, rZ * 1.5f };
      ParticleManager::GetInstance()->Emit(
          "explosion", pos, vel,
          Vector3{ 1.5f, 1.5f, 1.5f }, Vector3{ 0, 0, 0 }, 0.7f, Vector4{ 1, 1, 1, 1 });
  }

  // 1.5. GPU Particle（加点要素）: 敵撃破のたびに並列化されたEmit CSでバーストを発生させる
  GPUParticleManager::GetInstance()->EmitBurst(pos, 20);

  // 2. リングエフェクト
  if (texRing_) {
      auto device = Renderer::GetInstance()->GetDX()->GetDevice();
      EffectManager::GetInstance()->AddEffect(std::make_unique<RingEffect>(device, texRing_, pos));
  }

  // 3. 爆発のポイントライト（寿命付き）を生成
  auto lightObj = std::make_shared<AbsoluteEngine::GameObject>("ExplosionLight");
  lightObj->GetTransform().translate = pos;
  auto lightNodeComp = std::make_unique<AbsoluteEngine::LightNodeComponent>();
  lightNodeComp->type = AbsoluteEngine::LightNodeComponent::Type::Point;
  lightNodeComp->color = { 1.0f, 0.5f, 0.1f };  // オレンジ色の爆発光
  lightNodeComp->radius = 30.0f;
  lightNodeComp->decay = 1.0f;
  lightObj->AddComponent(std::move(lightNodeComp));
  auto lightComp = std::make_unique<ExplosionLightComponent>();
  lightComp->maxIntensity_ = 20.0f;
  lightComp->lifeTime_ = 0.5f;
  lightObj->AddComponent(std::move(lightComp));
  rootObjects_.push_back(lightObj);

  // 4. 旧型スケールアニメーションのヒットエフェクト（後方互換）
  HitEffect ef;
  ef.instance.Initialize({ resEffect_, {1, 1, 1, 1}, 0 });
  ef.position = pos;
  ef.frame = 0.0f;
  ef.isActive = true;
  hitEffects_.push_back(std::move(ef));
}

void GameScene::Update() {
  bool isTransitioning = false;
  const float deltaTime = 1.0f / 60.0f;
  time_ += deltaTime;

  // -----------------------------------------------------------------------
  // カメラシェイクの更新
  // -----------------------------------------------------------------------
  if (cameraShakeTimer_ < cameraShakeDuration_) {
      cameraShakeTimer_ += deltaTime;
      float t = 1.0f - (cameraShakeTimer_ / cameraShakeDuration_);
      float intensity = cameraShakeIntensity_ * t;
      float rX = ((rand() % 100) / 100.0f * 2.0f - 1.0f) * intensity;
      float rY = ((rand() % 100) / 100.0f * 2.0f - 1.0f) * intensity;
      cameraShakeOffset_ = { rX, rY, 0.0f };
  } else {
      cameraShakeOffset_ = { 0.0f, 0.0f, 0.0f };
  }

  // -----------------------------------------------------------------------
  // 画面歪み（RadialBlur）の更新
  // -----------------------------------------------------------------------
  if (hitDistortionTimer_ < hitDistortionDuration_) {
      hitDistortionTimer_ += deltaTime;
      float t = 1.0f - (hitDistortionTimer_ / hitDistortionDuration_);
      Renderer::GetInstance()->SetRadialBlurParam(radialBlurCenter_, hitDistortionIntensity_ * t);
      Renderer::GetInstance()->SetPostProcessMode(Renderer::PostProcessMode::RadialBlur);
  } else if (hitDistortionDuration_ > 0.0f) {
      Renderer::GetInstance()->SetRadialBlurParam(radialBlurCenter_, 0.0f);
      Renderer::GetInstance()->SetPostProcessMode(Renderer::PostProcessMode::Normal);
      hitDistortionDuration_ = 0.0f;
  }

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
          // ゲーム（レール）カメラ進行
          CameraContext ctx{};
          ctx.deltaTime = deltaTime;
          GetMainCamera()->SetContext(ctx);
          if (!isDragging && phase_ == GamePhase::InProgress) GetMainCamera()->Update(*services_.input);
      }

      // 骨のデバッグ表示トグル（Bキー）
      if (services_.input && services_.input->TriggerKey(DIK_B)) {
          showDebugSkeleton_ = !showDebugSkeleton_;
      }

  // -----------------------------------------------------------------------
  // SpawnManager の更新
  // 毎フレーム経過時間を計測し、トリガー時間を超えたSpawnEventの敵を生成する。
  // -----------------------------------------------------------------------
  if (phase_ == GamePhase::InProgress) {
      spawnTimer_ += deltaTime;

      for (auto& ev : spawnEvents_) {
          // 既にスポーン済み or まだトリガー時間に達していない場合はスキップ
          if (ev.spawned || spawnTimer_ < ev.triggerTime) continue;

          ev.spawned = true;

          // 敵のGameObjectを生成する
          auto newEnemy = std::make_shared<AbsoluteEngine::GameObject>("Enemy");
          newEnemy->SetTag("Enemy");

          // ディゾルブコンポーネント（撃破時の消滅エフェクト）
          newEnemy->AddComponent(std::make_unique<AbsoluteEngine::DissolveComponent>());

          // モデルコンポーネント
          auto modelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
          modelComp->LoadModel("resources/app/cube/cube.obj");
          newEnemy->AddComponent(std::move(modelComp));

          // 出現座標をセット
          newEnemy->GetTransform().translate = ev.position;

          // スフィアコライダー（当たり判定）
          auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
          colliderComp->type = AbsoluteEngine::ColliderComponent::Type::Sphere;
          colliderComp->radius = 2.0f;
          newEnemy->AddComponent(std::move(colliderComp));

          // EnemyComponent（死亡状態・ディゾルブ演出の制御）
          auto enemyComp = std::make_unique<EnemyComponent>();

          // -----------------------------------------------------------------------
          // Observer（イベントコールバック）の登録
          // Spawn 時点で GameScene 側の演出処理をラムダで登録する。
          // これにより EnemyComponent は演出の詳細を一切知らなくて済む（疎結合）。
          // -----------------------------------------------------------------------
          EnemyComponent* rawComp = enemyComp.get();
          enemyComp->onDestroyed = [this](const Vector3& hitPos) {
              // 1. ヒットエフェクト（パーティクル・リングエフェクト・爆発ライト）
              SpawnHitEffect(hitPos);

              // 2. カメラシェイク開始
              cameraShakeDuration_  = 0.3f;
              cameraShakeTimer_     = 0.0f;
              cameraShakeIntensity_ = 0.2f;

              // 3. 画面歪み（RadialBlur）開始
              hitDistortionDuration_  = 0.25f;
              hitDistortionTimer_     = 0.0f;
              hitDistortionIntensity_ = 0.06f;

              // 4. RadialBlur の中心を敵のスクリーン座標に設定
              auto* cam = isDebugCamera_
                  ? static_cast<Camera*>(debugCamera_.get())
                  : static_cast<Camera*>(GetMainCamera());
              if (cam) {
                  Matrix4x4 vp = Multiply(cam->GetViewMatrix(), cam->GetProjectionMatrix());
                  Vector3 sp = TransformPoint(hitPos, vp);
                  radialBlurCenter_ = { sp.x * 0.5f + 0.5f, -sp.y * 0.5f + 0.5f };
              }
          };
          newEnemy->AddComponent(std::move(enemyComp));

          rootObjects_.push_back(newEnemy);
      }
  }

  // -----------------------------------------------------------------------
  // ヒットエフェクトはイベントコールバック（onDestroyed）によって
  // EnemyComponent::Update 内から自動的に発火されるため、
  // ここでのポーリングは不要。
  // -----------------------------------------------------------------------


  // 寿命切れの ExplosionLight を削除
  for (auto it = rootObjects_.begin(); it != rootObjects_.end(); ) {
      auto& obj = *it;
      bool shouldDelete = false;
      if (obj && obj->GetName() == "ExplosionLight") {
          for (const auto& comp : obj->GetComponents()) {
              if (comp->GetTypeName() == "ExplosionLightComponent") {
                  if (static_cast<ExplosionLightComponent*>(comp.get())->IsDead()) {
                      shouldDelete = true;
                  }
                  break;
              }
          }
      }
      if (shouldDelete) {
          it = rootObjects_.erase(it);
      } else {
          ++it;
      }
  }

  // パーティクルの更新（爆発エフェクト等）
  ParticleManager::GetInstance()->Update(deltaTime);

  // EffectManagerの更新（RingEffectなど）
  auto* activeCamera = isDebugCamera_ ? static_cast<Camera*>(debugCamera_.get()) : static_cast<Camera*>(GetMainCamera());
  EffectManager::GetInstance()->Update(deltaTime, activeCamera);

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

  // ヒットエフェクト（旧型スケールアニメーション）の更新
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
      Vector3 nearPos{ 0,0,0 }, midPos{ 0,0,0 }, farPos{ 0,0,0 };

      // HUD に使うカメラ（デバッグカメラ or ゲームカメラ）
      Camera* hudCam = isDebugCamera_
          ? static_cast<Camera*>(debugCamera_.get())
          : static_cast<Camera*>(GetMainCamera());

      if (playerObj_) {
          if (auto pComp = playerObj_->GetComponent<PlayerComponent>()) {
              hp    = pComp->GetHp();
              maxHp = pComp->GetMaxHp();

              isLockingMode = pComp->lockon_.IsLockingMode();
              lockPositions = pComp->lockon_.GetLockedScreenPositions(GetMainCamera());

              // Animation補間（加点要素）：ロックオンのON/OFF切り替わり時に
              // 歩行アニメーション同士をクロスフェードで遷移させる。
              if (isLockingMode != wasLockingMode_) {
                  if (auto* playerModelComp = playerObj_->GetComponent<AbsoluteEngine::ModelComponent>()) {
                      if (auto* instance = playerModelComp->GetModelInstance()) {
                          instance->PlayAnimation(isLockingMode ? playerSneakWalkAnim_ : playerWalkAnim_, true, 0.3f);
                      }
                  }
                  wasLockingMode_ = isLockingMode;
              }

              // 3D レティクルのワールド座標を PlayerComponent から取得
              nearPos = pComp->GetNearReticleWorldPos();
              midPos  = pComp->GetMidReticleWorldPos();
              farPos  = pComp->GetFarReticleWorldPos();

              gameHUD_->Update(hp, maxHp, lockPositions, isLockingMode,
                               pComp->GetCursorPos(), hudCam, nearPos, midPos, farPos);
          }
      } else {
          gameHUD_->Update(hp, maxHp, lockPositions, isLockingMode,
                           { 640.0f, 360.0f }, hudCam, nearPos, midPos, farPos);
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

  Camera* activeCamera = nullptr;
  if (isDebugCamera_) {
      // Debug Camera ON: エディット中はEditorCamera、プレイ中はDebugCameraを使用する
      if (playMode_ == PlayMode::Edit && editorCamera_) {
          renderer->SetCamera(*editorCamera_);
          activeCamera = editorCamera_.get();
      } else {
          renderer->SetCamera(*debugCamera_);
          activeCamera = debugCamera_.get();
      }
  } else {
      // Debug Camera OFF: エディット/プレイ共通で GetMainCamera() を標準描画に使用する（タスクD）
      renderer->SetCamera(*GetMainCamera());
      activeCamera = GetMainCamera();
  }

  renderer->SetEnvironmentMap(skybox_.GetTexture());
  renderer->SetRandomParam(time_);

  // --- ライトの適用 ---
  ApplyEditorLightsToRenderer(renderer);

  // オブジェクトの描画
  skybox_.Draw();

  // エディタ上で配置したオブジェクト群の描画
  for (auto& obj : rootObjects_) {
      if (obj) obj->Draw();
  }

  for (auto& ef : hitEffects_) {
      renderer->DrawEffectModel(&ef.instance);
  }

  // EffectManager（RingEffect等）の描画
  EffectManager::GetInstance()->Draw();

  // パーティクルの描画（爆発エフェクト等）
  ParticleManager::GetInstance()->Draw(BlendMode::Add);

  // GPU Particleの描画（加点要素: GPU Particle拡張）
  Renderer::GetInstance()->DrawGPUParticles(BlendMode::Add);

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
          Vector3 camForward = { invView.m[2][0], invView.m[2][1], invView.m[2][2] };

          Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f}; // 黄色
          const int subdivisions = 100;
          Vector3 prevPoint = Spline::GetPoint(waypoints, 0.0f);
          for (int i = 1; i <= subdivisions; ++i) {
              float t = (float)i / (float)subdivisions;
              Vector3 currPoint = Spline::GetPoint(waypoints, t);
              
              Vector3 diff1 = { prevPoint.x - camEye.x, prevPoint.y - camEye.y, prevPoint.z - camEye.z };
              Vector3 diff2 = { currPoint.x - camEye.x, currPoint.y - camEye.y, currPoint.z - camEye.z };
              if (Dot(diff1, camForward) > 0.0f && Dot(diff2, camForward) > 0.0f) {
                  renderer->DrawLine(prevPoint, currPoint, color);
              }
              prevPoint = currPoint;
          }
          for (const auto& wp : waypoints) {
              Vector3 diff = { wp.x - camEye.x, wp.y - camEye.y, wp.z - camEye.z };
              if (Dot(diff, camForward) > 0.0f) {
                  renderer->DrawLine(Vector3{wp.x, wp.y - 1.0f, wp.z}, Vector3{wp.x, wp.y + 1.0f, wp.z}, Vector4{1, 0, 0, 1});
                  renderer->DrawLine(Vector3{wp.x - 1.0f, wp.y, wp.z}, Vector3{wp.x + 1.0f, wp.y, wp.z}, Vector4{1, 0, 0, 1});
              }
          }
      }
  }

  // グリッドの描画（進行感を演出）
  renderer->DrawGrid(500.0f, 50, Vector4{0.2f, 0.4f, 0.8f, 0.5f});

  // 骨のデバッグ表示（Bキートグル）
  if (showDebugSkeleton_ && playerObj_) {
      if (auto* modelComp = playerObj_->GetComponent<AbsoluteEngine::ModelComponent>()) {
          if (auto* instance = modelComp->GetModelInstance()) {
              instance->DrawSkeleton();
          }
      }
  }

  renderer->RenderPrimitives();

  // HUD等の2Dスプライト描画
  if (playMode_ == PlayMode::Play && gameHUD_) {
      gameHUD_->Draw();
  }

  Matrix4x4 projInverse;
  if (isDebugCamera_) {
      // Debug Camera ON: エディット中はEditorCamera、プレイ中はDebugCameraのプロジェクション逆行列を使用
      if (playMode_ == PlayMode::Edit && editorCamera_) projInverse = Inverse(editorCamera_->GetProjectionMatrix());
      else projInverse = Inverse(debugCamera_->GetProjectionMatrix());
  } else {
      // Debug Camera OFF: GetMainCamera() のプロジェクション逆行列を使用（タスクD）
      projInverse = Inverse(GetMainCamera()->GetProjectionMatrix());
  }

  renderer->EndRenderScene(projInverse);
}



void GameScene::DrawEditorUI() {
    BaseScene::DrawEditorUI(); // ツールバー等の描画

#ifdef USE_IMGUI
    if (phase_ != GamePhase::GameOver) {
        // [REMOVED] Duplicated ImGui::Begin("Viewport##GameView") which breaks ImGui rendering
    }

    // -----------------------------------------------------------------------
    // Debug Camera トグルはTimeline Editor内に一本化した（GetDebugCameraFlag経由）
    // ここにあった専用ツールバーウィンドウは廃止
    // -----------------------------------------------------------------------

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

    // SpawnManagerのデバッグ情報
    ImGui::SeparatorText("SpawnManager");
    ImGui::Text("Elapsed Time: %.2f sec", spawnTimer_);
    int spawnedCount = 0;
    for (const auto& ev : spawnEvents_) {
        if (ev.spawned) ++spawnedCount;
    }
    ImGui::Text("Spawned: %d / %d events", spawnedCount, (int)spawnEvents_.size());
    if (ImGui::Button("Reset SpawnTimer")) {
        spawnTimer_ = 0.0f;
        for (auto& ev : spawnEvents_) {
            ev.spawned = false;
        }
    }
    
    ImGui::SeparatorText("Camera Controls");
    Vector3 eye = GetMainCamera()->GetEye();
    Vector3 target = GetMainCamera()->GetTarget();
    ImGui::Text("Camera Eye: (%.2f, %.2f, %.2f)", eye.x, eye.y, eye.z);
    ImGui::Text("Camera Target: (%.2f, %.2f, %.2f)", target.x, target.y, target.z);
    
    if (showDebugRail_) {
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

    // Debug Cameraのトグルボタン自体はTimeline Editorに一本化したため、ここでは状態表示のみ行う
    if (isDebugCamera_) {
        ImGui::TextDisabled("Debug Camera: ON (Timeline Editorで切替)");
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