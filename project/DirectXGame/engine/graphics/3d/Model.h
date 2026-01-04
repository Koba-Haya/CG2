#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>

#include "DirectXCommon.h"
#include "Matrix.h"
#include "UnifiedPipeline.h"
#include "Vector.h"
#include "Method.h"

#include "ModelUtils.h" // ModelData, VertexData など
#include "TextureResource.h"

// ===== CB layouts (HLSLと揃える) =====
struct ModelMaterialCB {
    Vector4 color{ 1, 1, 1, 1 };
    int32_t enableLighting{ 1 };
    float pad[3]{ 0, 0, 0 };
    Matrix4x4 uvTransform = MakeIdentity4x4();
};

struct ModelTransformCB {
    Matrix4x4 WVP = MakeIdentity4x4();
    Matrix4x4 World = MakeIdentity4x4();
};

class Model {
public:
    struct CreateInfo {
        DirectXCommon* dx = nullptr;
        UnifiedPipeline* pipeline = nullptr;
        ModelData modelData;
        Vector4 baseColor = { 1, 1, 1, 1 };
        int32_t lightingMode = 1;
    };

    bool Initialize(const CreateInfo& ci);

    void SetWorldTransform(const Matrix4x4& world);
    void SetColor(const Vector4& color);

    void SetLightingMode(int32_t mode);
    void SetUVTransform(const Matrix4x4& uv);

    void Draw(const Matrix4x4& view, const Matrix4x4& proj, ID3D12Resource* directionalLightCB);

private:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D12Resource> CreateUploadBuffer(size_t size);

private:
    DirectXCommon* dx_ = nullptr;
    UnifiedPipeline* pipeline_ = nullptr;

    // 共有テクスチャ（SRVはTextureResourceがRAII管理）
    std::shared_ptr<TextureResource> texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE texSrvHandleGPU_{};

    ComPtr<ID3D12Resource> vb_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};

    ComPtr<ID3D12Resource> cbMaterial_;
    ModelMaterialCB* cbMatMapped_ = nullptr;

    ComPtr<ID3D12Resource> cbTransform_;
    ModelTransformCB* cbTransMapped_ = nullptr;

    Matrix4x4 world_ = MakeIdentity4x4();
    uint32_t vertexCount_ = 0;
};
