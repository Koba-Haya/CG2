#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "GPUParticle.h"
#include "Matrix.h"
#include "BlendMode.h" // ブレンドモード列挙体
#include <vector>

class DirectXCommon;
class TextureResource;

class GPUParticleManager {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    static GPUParticleManager* GetInstance();

    void Initialize(DirectXCommon* dx);
    void Update();
    // blendMode: 描画に使用するブレンドモード（デフォルト Alpha）
    void Draw(BlendMode blendMode = BlendMode::Alpha);

private:
    GPUParticleManager() = default;
    ~GPUParticleManager() = default;

    void CreateResources();
    void CreateComputePipeline();
    void CreateGraphicsPipeline();
    void CreateQuad();

    DirectXCommon* dx_ = nullptr;
    
    // Resource
    ComPtr<ID3D12Resource> particleBuffer_;
    uint32_t srvIndex_ = 0;
    uint32_t uavIndex_ = 0;

    // Pipelines
    ComPtr<ID3D12RootSignature> computeRootSignature_;
    ComPtr<ID3D12PipelineState> computePipelineState_;

    ComPtr<ID3D12RootSignature> emitRootSignature_;
    ComPtr<ID3D12PipelineState> emitPipelineState_;
    ComPtr<ID3D12PipelineState> updatePipelineState_;

    ComPtr<ID3D12RootSignature> graphicsRootSignature_;
    // ブレンドモードごとのパイプライン（インデックスは BlendMode の enum 値と対応）
    // [0]=Opaque相当, [1]=Alpha, [2]=Add, [3]=Subtract, [4]=Multiply, [5]=Screen
    static constexpr int kBlendModeCount = 6;
    ComPtr<ID3D12PipelineState> graphicsPipelineStates_[kBlendModeCount];

    // Constant Buffer for PerView
    struct PerView {
        Matrix4x4 viewProjection;
        Matrix4x4 billboardMatrix;
    };
    ComPtr<ID3D12Resource> perViewCB_;
    PerView* perViewMapped_ = nullptr;

    struct PerFrame {
        float time;
        float deltaTime;
    };
    ComPtr<ID3D12Resource> perFrameCB_;
    PerFrame* perFrameMapped_ = nullptr;

    ComPtr<ID3D12Resource> emitterCB_;
    EmitterSphere* emitterMapped_ = nullptr;

    ComPtr<ID3D12Resource> freeListIndexBuffer_;
    ComPtr<ID3D12Resource> freeListBuffer_;
    uint32_t freeListIndexUavIndex_ = 0;
    uint32_t freeListUavIndex_ = 0;

    std::unique_ptr<TextureResource> texture_;

    // Quad Mesh
    ComPtr<ID3D12Resource> vb_;
    ComPtr<ID3D12Resource> ib_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    static const uint32_t kMaxParticles = 1024;
};
