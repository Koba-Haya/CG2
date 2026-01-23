#pragma once

#include "FrameWork.h"

#include "Camera.h"
#include "Matrix.h"
#include "ModelInstance.h"
#include "ModelManager.h"
#include "ModelResource.h"
#include "ShaderCompiler.h"
#include "Sprite.h"
#include "Transform.h"
#include "UnifiedPipeline.h"

#include "ParticleEmitter.h"
#include "ParticleManager.h"

#include <fstream>
#include <memory>
#include <string>

class TextureResource;

// ===== DirectionalLight =====
struct DirectionalLight {
  Vector4 color;
  Vector3 direction;
  float intensity;
  int32_t enabled;
};

// ===== Camera =====
struct CameraForGPU {
  Vector3 worldPosition{};
  float pad = 0.0f;
};

// ===== PointLight =====
struct PointLight {
  Vector4 color;
  Vector3 position;
  float intensity;
  float radius;
  float decay;
  int32_t enabled;
  float padding[2];
};

// ===== SpotLight =====
struct SpotLight {
  Vector4 color;
  Vector3 position;
  float intensity;
  Vector3 direction;
  float distance;
  float decay;
  float cosAngle;
  int32_t enabled;
  float padding[2];
};

class GameApp final : public AbsoluteFrameWork {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  GameApp();
  ~GameApp() override;

protected:
  void Initialize() override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  void InitPipelines_();
  void InitResources_();
  void InitLogging_();
  void InitCamera_();

private:
  std::ofstream logStream_;

  ShaderCompiler shaderCompiler_;

  UnifiedPipeline objPipeline_;
  UnifiedPipeline emitterGizmoPipelineWire_;

  UnifiedPipeline spritePipelineAlpha_;
  UnifiedPipeline spritePipelineAdd_;
  UnifiedPipeline spritePipelineSub_;
  UnifiedPipeline spritePipelineMul_;
  UnifiedPipeline spritePipelineScreen_;

  UnifiedPipeline particlePipelineAlpha_;
  UnifiedPipeline particlePipelineAdd_;
  UnifiedPipeline particlePipelineSub_;
  UnifiedPipeline particlePipelineMul_;
  UnifiedPipeline particlePipelineScreen_;

  // ===== Model Resources (shared) =====
  std::shared_ptr<ModelResource> resSphere_;
  std::shared_ptr<ModelResource> resPlane_;
  std::shared_ptr<ModelResource> resCube_;
  std::shared_ptr<ModelResource> resTerrain_;

  // ===== Model Instances (per-object) =====
  ModelInstance modelSphere_;
  ModelInstance modelPlane_;
  ModelInstance modelEmitterSphere_;
  ModelInstance modelEmitterBox_;
  ModelInstance modelTerrain_;

  Sprite sprite_;

  // ===== Particle System =====
  static constexpr uint32_t kParticleCount_ = 300;
  std::string particleGroupName_ = "default";
  uint32_t initialParticleCount_ = 30;
  bool showEmitterGizmo_ = false;
  int particleBlendMode_ = 0;

  ParticleEmitter particleEmitter_;

  // ===== Constant Buffer =====
  ComPtr<ID3D12Resource> cameraCB_;
  CameraForGPU *cameraData_ = nullptr;

  ComPtr<ID3D12Resource> directionalLightCB_;
  DirectionalLight *directionalLightData_ = nullptr;
  bool enableDirectionalLight_ = false;

  ComPtr<ID3D12Resource> pointLightCB_;
  PointLight *pointLightData_ = nullptr;
  bool enablePointLight_ = true;

  ComPtr<ID3D12Resource> spotLightCB_;
  SpotLight *spotLightData_ = nullptr;
  bool enableSpotLight_ = false;

  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;
  Transform transform2_;
  Transform terrainTransform_;

  Matrix4x4 view3D_;
  Matrix4x4 proj3D_;

  std::unique_ptr<Camera> camera_;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;

  AccelerationField accelerationField_;
  bool enableAccelerationField_ = false;
};
