#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "BaseScene.h"

#include "Audio.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "Method.h"
#include "ModelInstance.h"
#include "ModelManager.h"
#include "ModelResource.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "ShaderCompiler.h"
#include "Sprite.h"
#include "Transform.h"
#include "UnifiedPipeline.h"
#include "Vector.h"

#include "Skybox.h"

// ===== Camera =====
struct CameraForGPU {
  Vector3 worldPosition{};
  float pad = 0.0f;
};

// ===== DirectionalLight =====
static constexpr int kMaxDirLights = 4;

struct alignas(16) DirectionalLightCB {
  float color[4];
  float direction[3];
  float intensity;
  int32_t enabled;
  float pad0[3];
};
static_assert(sizeof(DirectionalLightCB) == 48,
              "DirectionalLightCB size mismatch");

struct alignas(16) DirectionalLightGroupCB {
  int32_t count;
  float padCount[3];
  DirectionalLightCB lights[kMaxDirLights];
  int32_t enabled;
  float padEnabled[3];
};
static_assert(sizeof(DirectionalLightGroupCB) == 16 + 48 * kMaxDirLights + 16,
              "DirectionalLightGroupCB size mismatch");

// ===== PointLight =====
struct alignas(16) PointLightCB {
  float color[4];
  float position[3];
  float intensity;
  float radius;
  float decay;
  int32_t enabled;
  float pad0;
};
static_assert(sizeof(PointLightCB) == 48,
              "PointLightCB size mismatch (expect 48)");

static constexpr int kMaxPointLights = 16;

struct alignas(16) PointLightGroupCB {
  int32_t count;
  float padCount[3];
  PointLightCB lights[kMaxPointLights];
  int32_t enabled;
  float padEnabled[3];
};
static_assert(sizeof(PointLightGroupCB) == 16 + 48 * kMaxPointLights + 16,
              "PointLightGroupCB size mismatch");

// ===== SpotLight =====
static constexpr int kMaxSpotLights = 8;

struct alignas(16) SpotLightCB {
  float color[4];
  float position[3];
  float intensity;
  float direction[3];
  float distance;
  float decay;
  float cosAngle;
  int32_t enabled;
  float pad0;
};
static_assert(sizeof(SpotLightCB) == 64, "SpotLightCB size mismatch");

struct alignas(16) SpotLightGroupCB {
  int32_t count;
  float padCount[3];
  SpotLightCB lights[kMaxSpotLights];
  int32_t enabled;
  float padEnabled[3];
};
static_assert(sizeof(SpotLightGroupCB) == 16 + 64 * kMaxSpotLights + 16,
              "SpotLightGroupCB size mismatch");

class GameScene final : public BaseScene {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  GameScene() = default;
  ~GameScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw(ID3D12GraphicsCommandList *cmdList) override;

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

  UnifiedPipeline skyboxPipeline_;

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

  ComPtr<ID3D12Resource> cameraCB_;
  CameraForGPU *cameraData_ = nullptr;

  ComPtr<ID3D12Resource> directionalLightCB_;
  DirectionalLightGroupCB *directionalLightData_ = nullptr;
  bool enableDirectionalLight_ = false;

  ComPtr<ID3D12Resource> pointLightCB_;
  PointLightGroupCB *pointLightsData_ = nullptr;
  bool enablePointLight_ = true;

  ComPtr<ID3D12Resource> spotLightCB_;
  SpotLightGroupCB *spotLightData_ = nullptr;
  bool enableSpotLight_ = false;

  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;
  Transform transform2_;
  Transform terrainTransform_;

  Matrix4x4 view3D_{};
  Matrix4x4 proj3D_{};

  std::unique_ptr<Camera> camera_;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;

  AccelerationField accelerationField_{};
  bool enableAccelerationField_ = false;
};
