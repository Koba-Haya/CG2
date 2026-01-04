// GameApp.h
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

#include <fstream>
#include <list>
#include <memory>
#include <string>

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "xaudio2.lib")

class TextureResource;

// ===== Light (CB用) =====
// HLSL側の定義と揃える（float4 color; float3 dir; float intensity;）
struct DirectionalLight {
    Vector4 color{ 1, 1, 1, 1 };
    Vector3 direction{ 0, -1, 0 };
    float intensity{ 1.0f };
};

// パーティクル
struct Particle {
    Transform transform;
    Vector3 velocity;
    float lifetime;
    float age;
    Vector4 color;
};

// 加速度フィールド
struct AccelerationField {
    Vector3 acceleration;
    AABB area;
};

// 形状
enum class EmitterShape {
    Box,
    Sphere
};

enum class ParticleColorMode {
    RandomRGB,
    RangeRGB,
    RangeHSV,
    Fixed
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

    void RespawnParticle_(Particle& p);
    Vector4 GenerateParticleColor_();

    Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate,
        const Matrix4x4& viewMatrix);
    bool IsCollision(const AABB& aabb, const Vector3& point);

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

    // ===== Particle Instancing =====
    ComPtr<ID3D12Resource> particleInstanceBuffer_;
    D3D12_GPU_DESCRIPTOR_HANDLE particleMatricesSrvGPU_{};

    struct ParticleForGPU {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Vector4 color;
    };
    ParticleForGPU* particleMatrices_ = nullptr;

    ComPtr<ID3D12Resource> particleMaterialCB_;
    struct ParticleMaterialData {
        Vector4 color;
        int32_t enableLighting;
        float pad[3];
        Matrix4x4 uvTransform;
    };
    ParticleMaterialData* particleMaterialData_ = nullptr;

    static constexpr uint32_t kParticleCount_ = 100;

    std::list<Particle> particles_;
    uint32_t activeParticleCount_ = 0;

    SrvHandle particleInstanceSrv_;

    uint32_t particleCountUI_ = kParticleCount_;
    uint32_t initialParticleCount_ = 30;

    Vector3 particleSpawnCenter_{ 0.0f, 0.0f, 0.0f };
    Vector3 particleSpawnExtent_{ 1.0f, 1.0f, 1.0f };

    Vector3 particleBaseDir_{ 0.0f, 1.0f, 0.0f };
    float particleDirRandomness_ = 0.5f;

    float particleSpeedMin_ = 0.5f;
    float particleSpeedMax_ = 2.0f;

    float particleLifeMin_ = 1.0f;
    float particleLifeMax_ = 3.0f;

    float particleEmitRate_ = 10.0f;
    float particleEmitAccum_ = 0.0f;

    EmitterShape emitterShape_ = EmitterShape::Box;
    bool showEmitterGizmo_ = false;

    ParticleColorMode particleColorMode_ = ParticleColorMode::RandomRGB;
    Vector4 particleBaseColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 particleColorRangeRGB_{ 0.2f, 0.2f, 0.2f };
    Vector3 particleBaseHSV_{ 0.0f, 1.0f, 1.0f };
    Vector3 particleHSVRange_{ 0.1f, 0.1f, 0.1f };

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

    D3D12_VERTEX_BUFFER_VIEW particleVBView_{};
    D3D12_INDEX_BUFFER_VIEW particleIBView_{};
    ComPtr<ID3D12Resource> particleVB_;
    ComPtr<ID3D12Resource> particleIB_;

    std::shared_ptr<TextureResource> particleTextureResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE particleTextureHandle_{};

    AccelerationField accelerationField_;
    bool enableAccelerationField_ = false;
};
