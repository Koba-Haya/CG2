#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include <vector>
#include "GPUParticle.h"
#include "Matrix.h"

class DirectXCommon;

class GPUParticleManager {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    static GPUParticleManager* GetInstance();

    void Initialize(DirectXCommon* dx);
    void Update();
    void Draw();

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

    ComPtr<ID3D12RootSignature> graphicsRootSignature_;
    ComPtr<ID3D12PipelineState> graphicsPipelineState_;

    // Constant Buffer for PerView
    struct PerView {
        Matrix4x4 viewProjection;
        Matrix4x4 billboardMatrix;
    };
    ComPtr<ID3D12Resource> perViewCB_;
    PerView* perViewMapped_ = nullptr;

    // Quad Mesh
    ComPtr<ID3D12Resource> vb_;
    ComPtr<ID3D12Resource> ib_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    static const uint32_t kMaxParticles = 1024;
};
