#pragma once
#include "BaseScene.h"
#include "GameCamera.h"
#include "Skybox.h"
#include "DebugCamera.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "Renderer.h"
#include "graphics/texture/RenderTexture.h"
#include "graphics/texture/DepthTexture.h"
#include "../actor/Player.h"
#include "../actor/Bullet.h"
#include "../actor/Enemy.h"
#include <memory>
#include <vector>

class GameScene final : public BaseScene {
public:
  GameScene() = default;
  ~GameScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  void SpawnHitEffect(const Vector3 &pos);

private:
  std::unique_ptr<GameCamera> gameCamera_;
  std::unique_ptr<DebugCamera> debugCamera_;
  std::unique_ptr<RenderTexture> renderTexture_;
  std::unique_ptr<DepthTexture> depthTexture_;
  std::unique_ptr<RenderTexture> postProcessTexture_;
  std::unique_ptr<RenderTexture> gaussianTempTexture_;
  Renderer::PostProcessMode postProcessMode_ = Renderer::PostProcessMode::Normal;
  float vignetteScale_ = 16.0f;
  float vignettePow_ = 0.8f;
  int32_t boxFilterK_ = 1;
  int32_t gaussianFilterK_ = 1;
  float gaussianFilterSigma_ = 1.0f;
  float hsvHue_ = 0.0f;
  float hsvSaturation_ = 0.0f;
  float hsvValue_ = 0.0f;
  class RailCameraController* railController_ = nullptr; // 所有権は gameCamera_ が持つ


  bool isDebugCamera_ = false;
  bool showDebugRail_ = true;

  Skybox skybox_;

  // アクター関連
  Player player_;
  std::vector<Bullet> bullets_;
  std::vector<Enemy> enemies_;

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

  float shootCooldown_ = 0.0f;
  float time_ = 0.0f;
};