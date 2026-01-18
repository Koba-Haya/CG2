#pragma once

#include "FrameWork.h"

#include "Camera.h"
#include "Matrix.h"
#include "Transform.h"
#include "UnifiedPipeline.h"
#include "Model.h"
#include "Sprite.h"
#include "ShaderCompiler.h"

#include "ParticleEmitter.h"
#include "ParticleManager.h"

#include <fstream>
#include <memory>
#include <string>

class TextureResource;

// ===== Light (CB用) =====
struct DirectionalLight {
	Vector4 color{ 1, 1, 1, 1 };
	Vector3 direction{ 0, -1, 0 };
	float intensity{ 1.0f };
};

// ===== Camera (CB用) =====
struct CameraForGPU {
	Vector3 worldPosition{};
	float pad = 0.0f;
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
	DirectionalLight* directionalLightData_ = nullptr;

	ComPtr<ID3D12Resource> cameraCB_;
	CameraForGPU* cameraData_ = nullptr;

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
