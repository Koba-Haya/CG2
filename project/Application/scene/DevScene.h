#pragma once
#include "AnimationManager.h"
#include "BaseScene.h"
#include "Camera.h"
#include "Cylinder.h"
#include "LightTypes.h"
#include "Matrix.h"
#include "Method.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Ring.h"
#include "Skybox.h"
#include "Sprite.h"
#include "Transform.h"
#include "Vector.h"
#include "graphics/texture/TextureResource.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class DevScene final : public BaseScene {
public:
  DevScene() = default;
  ~DevScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  void InitResources_();
  void InitLogging_();
  void InitCamera_();
  void SpawnHitEffect(const Vector3 &pos);

private:
  std::ofstream logStream_;

  std::shared_ptr<ModelResource> resSphere_;
  std::shared_ptr<ModelResource> resCube_;
  std::shared_ptr<ModelResource> resAnimCube_;

  ModelInstance modelSphere_;
  ModelInstance modelEmitterSphere_;
  ModelInstance modelEmitterBox_;
  ModelInstance modelAnimCube_;
  std::shared_ptr<Animation> animCubeAnim_;

  std::shared_ptr<ModelResource> resSimpleSkin_;
  std::shared_ptr<ModelResource> resHuman_;
  ModelInstance modelSimpleSkin_;
  ModelInstance modelHuman_;
  std::shared_ptr<Animation> animSimpleSkin_;
  std::shared_ptr<Animation> animHuman_;

  Transform transformAnimCube_;
  Transform transformSimpleSkin_;
  Transform transformHuman_;
  bool showSkeleton_ = false;

  Sprite sprite_;
  Skybox skybox_;

  // 反射設定用
  bool enableReflection_ = false;
  float reflectionWeight_ = 0.5f;

  // パーティクル関連
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
  int particleBlendMode_ = 1;

  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;

  std::unique_ptr<Camera> camera_;

  // エフェクト管理
  struct HitEffect {
    ModelInstance instance;
    float frame = 0.0f;
    float maxFrame = 30.0f;
    bool isActive = false;
    Vector3 position;
  };

  std::shared_ptr<ModelResource> resEffect_;
  std::vector<HitEffect> hitEffects_;

  // 常時確認用プリミティブ
  Ring ring_;
  std::shared_ptr<TextureResource> texRing_;
  Ring::Params ringParams_;
  Transform ringTransform_;
  Vector2 ringUVScale_ = {1.0f, 1.0f};

  Cylinder cylinder_;
  std::shared_ptr<TextureResource> texCylinder_;
  Cylinder::Params cylinderParams_;
  Transform cylinderTransform_;
  Vector2 cylinderUVScale_ = {1.0f, 1.0f};
};
