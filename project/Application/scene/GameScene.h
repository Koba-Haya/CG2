#pragma once
#include "BaseScene.h"
#include "GameCamera.h"
#include "Skybox.h"
#include "DebugCamera.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "Renderer.h"
#include "../actor/Player/PlayerComponent.h"
#include "../actor/Bullet/BulletComponent.h"
#include "../actor/Enemy/EnemyShootComponent.h"
#include "../hud/GameHUD.h"
#include <memory>
#include <vector>

enum class GamePhase {
  InProgress,
  Boss,
  Clear,
  GameOver
};

class GameScene final : public BaseScene {
public:
  GameScene() = default;
  ~GameScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;
  void DrawEditorUI() override;





private:
  void SpawnHitEffect(const Vector3 &pos);

private:
  std::unique_ptr<DebugCamera> debugCamera_;

  bool isDebugCamera_ = false;
  bool showDebugRail_ = true;

  std::string sceneFilePath_ = "C:/Users/haya2/source/repos/CG2/project/Application/resources/editor/scene.json";

  Skybox skybox_;

  // アクター関連
  std::shared_ptr<AbsoluteEngine::GameObject> playerObj_;

  // リソース
  std::shared_ptr<ModelResource> resPlayer_;
  std::shared_ptr<ModelResource> resBullet_;
  std::shared_ptr<ModelResource> resEnemy_;
  std::shared_ptr<ModelResource> resEffect_;

  // ヒットエフェクト
  struct HitEffect {
    HitEffect() = default;
    ~HitEffect() = default;
    HitEffect(HitEffect &&) noexcept = default;
    HitEffect &operator=(HitEffect &&) noexcept = default;

    ModelInstance instance;
    float frame = 0.0f;
    float maxFrame = 20.0f;
    bool isActive = false;
    Vector3 position;
  };
  std::vector<HitEffect> hitEffects_;

  float spawnTimer_ = 0.0f;
  
  float time_ = 0.0f;

  GamePhase phase_ = GamePhase::InProgress;

  std::unique_ptr<GameHUD> gameHUD_;
};