#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "LightTypes.h"
#include "Matrix.h"
#include "Method.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Skybox.h"
#include "Sprite.h"
#include "Transform.h"
#include "Vector.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
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
  void InitResources_();
  void InitLogging_();
  void InitCamera_();

private:
  std::ofstream logStream_;

  std::shared_ptr<ModelResource> resSphere_;
  std::shared_ptr<ModelResource> resCube_;

  ModelInstance modelSphere_;
  ModelInstance modelEmitterSphere_;
  ModelInstance modelEmitterBox_;

  Sprite sprite_;
  Skybox skybox_;

  // 反射設定用
  bool enableReflection_ = false; // デフォルト OFF
  float reflectionWeight_ = 0.5f; // 反射の強さ

  // その他既存のメンバ
  static constexpr uint32_t kParticleCount_ = 300;
  std::string particleGroupName_ = "default";
  uint32_t initialParticleCount_ = 30;
  bool showEmitterGizmo_ = false;
  bool enableAccelerationField_ = false;
  AccelerationField accelerationField_;
  ParticleEmitter particleEmitter_;

  std::vector<DirLight> dirLights_;
  bool enableDirectionalLight_ = false;
  std::vector<PointLight> pointLights_;
  bool enablePointLight_ = true;
  std::vector<SpotLight> spotLights_;
  bool enableSpotLight_ = false;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;
  int particleBlendMode_ = 1; // デフォルト加算 (BlendMode::Add)

  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;

  std::unique_ptr<Camera> camera_;

  struct HitEffect {
    ModelInstance instance;
    float frame = 0.0f;
    float maxFrame = 30.0f; // 0.5秒で消える
    bool isActive = false;
    Vector3 position;
  };

  std::shared_ptr<ModelResource> resEffect_; // particle.obj
  std::vector<HitEffect> hitEffects_;
  void SpawnHitEffect(const Vector3 &pos);
};