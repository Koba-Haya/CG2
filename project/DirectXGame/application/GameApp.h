#pragma once

#include "AABB.h"
#include "Audio.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "Matrix.h"
#include "Model.h"
#include "ShaderCompiler.h"
#include "Sprite.h"
#include "Transform.h"
#include "UnifiedPipeline.h"
#include "WinApp.h"
#include "SrvHandle.h"

// Particle system (Manager + Emitter)
#include "ParticleEmitter.h"
#include "ParticleManager.h"

#include <fstream>
#include <memory>
#include <string>

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "xaudio2.lib")

class TextureResource;

// ===== Light (CB用) =====
struct DirectionalLight {
    Vector4 color{ 1, 1, 1, 1 };
    Vector3 direction{ 0, -1, 0 };
    float intensity{ 1.0f };
};

class GameApp {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    GameApp();
    ~GameApp();

    bool Initialize();
    int Run();
    void Finalize();

private:
    void Update();
    void Draw();

    void InitPipelines_();
    void InitResources_();
    void InitLogging_();
    void InitCamera_();

private:
    WinApp winApp_;
    DirectXCommon dx_;
    Input input_;
    AudioManager audio_;
    ImGuiManager imgui_;

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
    //char particleGroupNameBuf_[64] = "default";
    uint32_t particleCountUI_ = kParticleCount_;
    uint32_t initialParticleCount_ = 30;
    bool showEmitterGizmo_ = false;

    ParticleEmitter particleEmitter_;

    ComPtr<ID3D12Resource> directionalLightCB_;
    DirectionalLight* directionalLightData_ = nullptr;

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
    int particleBlendMode_ = 0;
    bool useMonsterBall_ = true;
    float selectVol_ = 1.0f;

    AccelerationField accelerationField_;
    bool enableAccelerationField_ = false;
};
