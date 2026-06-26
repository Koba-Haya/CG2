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
#include "Renderer.h"
#include "AbsoluteEngine/audio/Audio.h"
#include "component/ExplosionLightComponent.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "GameObject.h"
#include "AbsoluteEngine/editor/EditorUIManager.h"


class DevScene final : public BaseScene {
public:
  DevScene();
  ~DevScene() override;

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
  std::shared_ptr<ModelResource> resTerrain_;

  ModelInstance modelSphere_;
  ModelInstance modelTerrain_;
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
  AccelerationField accelerationField_{};
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
  Transform transformTerrain_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;

  // --- 照準(レティクル)用 ---
  Sprite reticleSprite_;
  std::shared_ptr<TextureResource> texReticle_;
  Vector2 reticlePos_ = {640.0f, 360.0f};
  Vector2 reticleSize_ = {64.0f, 64.0f};
  float reticleSpeed_ = 500.0f; // 少し遅くした
  float snapPullSpeed_ = 350.0f; // 敵の中心に引っ張る力(プレイヤーの速度より低くして振り切れるようにする)
  
  float fireTimer_ = 0.0f;
  float fireInterval_ = 0.25f; // 発射間隔を少し遅く調整

  // カメラシェイク用
  float cameraShakeTimer_ = 0.0f;
  float cameraShakeDuration_ = 0.0f;
  float cameraShakeIntensity_ = 0.0f;
  Vector3 cameraShakeOffset_{0,0,0};

  // 画面歪み（RadialBlur）用
  float hitDistortionTimer_ = 0.0f;
  float hitDistortionDuration_ = 0.0f;
  float hitDistortionIntensity_ = 0.0f;

  // エネミースポーン用
  float enemySpawnTimer_ = 0.0f;
  float enemySpawnInterval_ = 2.0f;
  const int maxEnemies_ = 5;

  std::shared_ptr<ModelResource> resEffect_;
  std::shared_ptr<TextureResource> texRing_;
  std::shared_ptr<TextureResource> texCylinder_;

  std::shared_ptr<TextureResource> texNoise0_;
  float time_ = 0.0f;
  Vector2 radialBlurCenter_ = {0.5f, 0.5f};

  float hsvHue_ = 0.0f;
  float hsvSaturation_ = 0.0f;
  float hsvValue_ = 0.0f;

  std::string debugStr_;
};
