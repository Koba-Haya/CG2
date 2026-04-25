#pragma once

#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl.h>

#include "Matrix.h"
#include "Method.h"
#include "UnifiedPipeline.h"
#include "LightTypes.h"

class DirectXCommon;
class ModelInstance;
class Sprite;
class Skybox;
class ParticleManager;
class Camera;

// ===== GPU 定数バッファ型 (エンジン内部用 ── シーン層は使わない) =====

struct CameraForGPU {
  Vector3 worldPosition{};
  float pad = 0.0f;
};

// --- DirectionalLight CB ---
static constexpr int kMaxDirLights = 4;

struct alignas(16) DirectionalLightCB {
  float color[4];
  float direction[3];
  float intensity;
  int32_t enabled;
  float pad0[3];
};

struct alignas(16) DirectionalLightGroupCB {
  int32_t count;
  float padCount[3];
  DirectionalLightCB lights[kMaxDirLights];
  int32_t enabled;
  float padEnabled[3];
};

// --- PointLight CB ---
struct alignas(16) PointLightCB {
  float color[4];
  float position[3];
  float intensity;
  float radius;
  float decay;
  int32_t enabled;
  float pad0;
};

static constexpr int kMaxPointLights = 16;

struct alignas(16) PointLightGroupCB {
  int32_t count;
  float padCount[3];
  PointLightCB lights[kMaxPointLights];
  int32_t enabled;
  float padEnabled[3];
};

// --- SpotLight CB ---
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

struct alignas(16) SpotLightGroupCB {
  int32_t count;
  float padCount[3];
  SpotLightCB lights[kMaxSpotLights];
  int32_t enabled;
  float padEnabled[3];
};

// =================================================================
// Renderer ── 描画パイプラインの中央管理
// =================================================================
class Renderer {
public:
    static Renderer* GetInstance() {
        static Renderer inst;
        return &inst;
    }

    void Initialize(DirectXCommon* dx);

    // ===== 高レベル API (シーン層向け) =====

    /// Camera オブジェクトから view/proj + カメラ位置を一括セット
    void SetCamera(const Camera& camera);

    /// ライト設定（高レベル記述子ベース）
    void SetDirectionalLights(const std::vector<DirLight>& lights, bool groupEnabled);
    void SetPointLights(const std::vector<PointLight>& lights, bool groupEnabled);
    void SetSpotLights(const std::vector<SpotLight>& lights, bool groupEnabled);

    /// 画面情報
    float GetScreenWidth() const;
    float GetScreenHeight() const;
    float GetAspectRatio() const;

    // ===== エンジン内部向けアクセサ =====
    DirectXCommon* GetDX() const { return dx_; }
    const Matrix4x4& GetViewMatrix() const { return view_; }
    const Matrix4x4& GetProjectionMatrix() const { return proj_; }

    // ===== 描画メソッド群 =====
    void DrawModel(ModelInstance* model);
    void DrawSprite(Sprite* sprite);
    void DrawSkybox(Skybox* skybox);
    void DrawParticles(ParticleManager* pm, BlendMode blendMode = BlendMode::Alpha);

private:
    Renderer() = default;

    DirectXCommon* dx_ = nullptr;

    Matrix4x4 view_ = MakeIdentity4x4();
    Matrix4x4 proj_ = MakeIdentity4x4();

    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D12Resource> CreateUploadBuffer_(size_t size);

    ComPtr<ID3D12Resource> cameraCB_;
    CameraForGPU* cameraMapped_ = nullptr;

    ComPtr<ID3D12Resource> directionalLightCB_;
    DirectionalLightGroupCB* directionalLightMapped_ = nullptr;

    ComPtr<ID3D12Resource> pointLightCB_;
    PointLightGroupCB* pointLightMapped_ = nullptr;

    ComPtr<ID3D12Resource> spotLightCB_;
    SpotLightGroupCB* spotLightMapped_ = nullptr;

    // --- パイプライン群 ---
    std::unique_ptr<UnifiedPipeline> objPipelineOpaque_;
    std::unique_ptr<UnifiedPipeline> objPipelineWireframe_;
    std::unique_ptr<UnifiedPipeline> skyboxPipeline_;
    
    std::unique_ptr<UnifiedPipeline> spritePipelineAlpha_;
    std::unique_ptr<UnifiedPipeline> spritePipelineAdd_;
    std::unique_ptr<UnifiedPipeline> spritePipelineSub_;
    std::unique_ptr<UnifiedPipeline> spritePipelineMul_;
    std::unique_ptr<UnifiedPipeline> spritePipelineScreen_;

    std::unique_ptr<UnifiedPipeline> particlePipelineAlpha_;
    std::unique_ptr<UnifiedPipeline> particlePipelineAdd_;
    std::unique_ptr<UnifiedPipeline> particlePipelineSub_;
    std::unique_ptr<UnifiedPipeline> particlePipelineMul_;
    std::unique_ptr<UnifiedPipeline> particlePipelineScreen_;

    UnifiedPipeline* GetSpritePipeline_(BlendMode mode);
    UnifiedPipeline* GetParticlePipeline_(BlendMode mode);
};
