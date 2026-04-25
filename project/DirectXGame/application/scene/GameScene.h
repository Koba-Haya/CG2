#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "BaseScene.h"

#include "Camera.h"
#include "LightTypes.h"
#include "Matrix.h"
#include "Method.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Sprite.h"
#include "Skybox.h"
#include "Transform.h"
#include "Vector.h"

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
  std::shared_ptr<ModelResource> resPlane_;
  std::shared_ptr<ModelResource> resCube_;
  std::shared_ptr<ModelResource> resTerrain_;

  ModelInstance modelSphere_;
  ModelInstance modelPlane_;
  ModelInstance modelEmitterSphere_;
  ModelInstance modelEmitterBox_;
  ModelInstance modelTerrain_;

  Sprite sprite_;

  Skybox skybox_;

  static constexpr uint32_t kParticleCount_ = 300;
  std::string particleGroupName_ = "default";
  uint32_t initialParticleCount_ = 30;
  bool showEmitterGizmo_ = false;

  ParticleEmitter particleEmitter_;

  // ライト（高レベル記述子）
  std::vector<DirLight> dirLights_;
  bool enableDirectionalLight_ = false;

  std::vector<PointLight> pointLights_;
  bool enablePointLight_ = true;

  std::vector<SpotLight> spotLights_;
  bool enableSpotLight_ = false;

  // Transform
  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;
  Transform transform2_;
  Transform terrainTransform_;

  std::unique_ptr<Camera> camera_;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;

  AccelerationField accelerationField_{};
  bool enableAccelerationField_ = false;
};
