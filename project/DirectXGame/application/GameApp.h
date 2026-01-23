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

// ===== Camera =====
struct CameraForGPU {
  Vector3 worldPosition{};
  float pad = 0.0f;
};

// ===== DirectionalLight =====
static constexpr int kMaxDirLights = 4;

struct alignas(16) DirectionalLightCB {
  float color[4];     // float4
  float direction[3]; // float3
  float intensity;    // float
  int32_t enabled;    // int
  float pad0[3];      // 16byte境界
};

static_assert(sizeof(DirectionalLightCB) == 48,
              "DirectionalLightCB size mismatch");

struct alignas(16) DirectionalLightGroupCB {
  int32_t count;     // int
  float padCount[3]; // float3
  DirectionalLightCB lights[kMaxDirLights];
  int32_t enabled;     // int
  float padEnabled[3]; // float3
};

static_assert(sizeof(DirectionalLightGroupCB) == 16 + 48 * kMaxDirLights + 16,
              "DirectionalLightGroupCB size mismatch");

// ===== PointLight =====
struct alignas(16) PointLightCB {
  float color[4];    // float4
  float position[3]; // float3
  float intensity;   // float
  float radius;      // float
  float decay;       // float
  int32_t enabled;   // int
  float pad0;        // float (16byte揃え)
};

static constexpr int kMaxPointLights = 16;

struct alignas(16) PointLightGroupCB {
  int32_t count;     // int
  float padCount[3]; // float3
  PointLightCB lights[kMaxPointLights];
  int32_t enabled;     // HLSLに合わせて追加
  float padEnabled[3]; // 16byte揃え
};

// サイズチェック（環境依存ズレを潰す）
static_assert(sizeof(PointLightCB) == 48,
              "PointLightCB size mismatch (expect 48)");
static_assert(sizeof(PointLightGroupCB) == 16 + 48 * kMaxPointLights + 16,
              "PointLightGroupCB size mismatch");

// ===== SpotLight =====
static constexpr int kMaxSpotLights = 8;

struct alignas(16) SpotLightCB {
  float color[4];     // float4
  float position[3];  // float3
  float intensity;    // float
  float direction[3]; // float3
  float distance;     // float
  float decay;        // float
  float cosAngle;     // float
  int32_t enabled;    // int
  float pad0;         // 16byte境界
};

static_assert(sizeof(SpotLightCB) == 64, "SpotLightCB size mismatch");

struct alignas(16) SpotLightGroupCB {
  int32_t count;     // int
  float padCount[3]; // float3
  SpotLightCB lights[kMaxSpotLights];
  int32_t enabled;     // int
  float padEnabled[3]; // float3
};

static_assert(sizeof(SpotLightGroupCB) == 16 + 64 * kMaxSpotLights + 16,
              "SpotLightGroupCB size mismatch");
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

  Matrix4x4 view3D_;
  Matrix4x4 proj3D_;

  std::unique_ptr<Camera> camera_;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;

  AccelerationField accelerationField_;
  bool enableAccelerationField_ = false;
};
