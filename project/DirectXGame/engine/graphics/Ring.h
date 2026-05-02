#pragma once
#include "Vector.h"
#include "Matrix.h"
#include <vector>
#include <wrl.h>
#include <d3d12.h>

class Ring {
public:
    struct Vertex {
        Vector4 position;
        Vector2 texcoord;
        Vector4 color;
    };

    struct Params {
        uint32_t divide = 32;
        float outerRadius = 1.0f;
        float innerRadius = 0.5f;
        float startAngle = 0.0f;
        float endAngle = 3.14159265f * 2.0f;
        bool uvVertical = false; // false: Horizon, true: Vertical
        Vector4 colorInner = { 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 colorOuter = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

public:
    Ring() = default;
    ~Ring() = default;

    void Initialize(ID3D12Device* device, const Params& params);
    void Update(ID3D12Device* device, const Params& params);
    void Draw(ID3D12GraphicsCommandList* cmdList);

    void SetTransform(const Matrix4x4& world, const Matrix4x4& view, const Matrix4x4& proj);
    void SetMaterial(const Vector4& color, const Matrix4x4& uvTransform);

    ID3D12Resource* GetVertexBuffer() const { return vertexBuffer_.Get(); }
    ID3D12Resource* GetIndexBuffer() const { return indexBuffer_.Get(); }
    UINT GetIndexCount() const { return indexCount_; }

    D3D12_GPU_VIRTUAL_ADDRESS GetTransformCBAddress() const { return transformCB_->GetGPUVirtualAddress(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialCBAddress() const { return materialCB_->GetGPUVirtualAddress(); }

private:
    void CreateBuffers_(ID3D12Device* device);
    void GenerateVertices_();

private:
    Params params_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW ibView_{};
    UINT indexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformCB_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_;
    
    struct TransformCB {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };
    
    struct MaterialCB {
        Vector4 color;
        int enableLighting;
        Matrix4x4 uvTransform;
    };
};
