#pragma once

#include "FrameWork.h"

#include "Camera.h"
#include "Matrix.h"
#include "Model.h"
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
  Vector4 color;     // ライトの色
  Vector3 direction; // ライトの向き
  float intensity;   // 光の強さ
  int32_t enabled;   // 0:OFF 1:ON
};

// ===== Camera =====
struct CameraForGPU {
  Vector3 worldPosition{};
  float pad = 0.0f;
};

// ===== PointLight =====
struct PointLight {
  Vector4 color;    // ライトの色
  Vector3 position; // ライトの位置
  float intensity;  // 光の強さ
  float radius;     // 光の届く距離
  float decay;      // 減衰率
  int32_t enabled;  // 0:OFF 1:ON
  float padding[2]; // パディング
};

// ===== SpotLight =====
struct SpotLight {
  Vector4 color;     // ライトの色
  Vector3 position;  // ライトの位置
  float intensity;   // 光の強さ
  Vector3 direction; // ライトの向き
  float distance;    // 光の届く距離
  float decay;       // 減衰率
  float cosAngle;    // ライトの余弦
  int32_t enabled;   // 0:OFF 1:ON
  float padding[2];  // パディング
};

class GameApp final : public AbsoluteFrameWork {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  GameApp();
  ~GameApp() override;

protected:
  // Framework のフック
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

  Model model_;
  Model planeModel_;
  Sprite sprite_;
  Model emitterSphereModel_;
  Model emitterBoxModel_;

  // ===== Particle System =====
  static constexpr uint32_t kParticleCount_ = 300;
  std::string particleGroupName_ = "default";
  uint32_t initialParticleCount_ = 30;
  bool showEmitterGizmo_ = false;
  int particleBlendMode_ = 0;

  ParticleEmitter particleEmitter_;

  // ===== Constant Buffer =====
  ComPtr<ID3D12Resource> directionalLightCB_;
  DirectionalLight *directionalLightData_ = nullptr;
  bool enableDirectionalLight_ = false;

  ComPtr<ID3D12Resource> cameraCB_;
  CameraForGPU *cameraData_ = nullptr;

  ComPtr<ID3D12Resource> pointLightCB_;
  PointLight *pointLightData_ = nullptr;
  bool enablePointLight_ = true;

  ComPtr<ID3D12Resource> spotLightCB_;
  SpotLight *spotLightData_ = nullptr;
  bool enableSpotLight_ = true;

  Transform transform_;
  Transform cameraTransform_;
  Transform transformSprite_;
  Transform uvTransformSprite_;
  Transform transform2_;

  Matrix4x4 view3D_;
  Matrix4x4 proj3D_;

  std::unique_ptr<Camera> camera_;

  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;

  AccelerationField accelerationField_;
  bool enableAccelerationField_ = false;
};
