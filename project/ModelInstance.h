#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <memory>

#include "Matrix.h"
#include "Method.h"
#include "Vector.h"

class DirectXCommon;
class UnifiedPipeline;
class ModelResource;

// ModelResource(共有) を描画する「インスタンス」
// - 個体ごとの World / 色 / LightingMode をここで持つ
// - GPUのVB/Textureは ModelResource から参照する
class ModelInstance {
public:
    struct CreateInfo {
        DirectXCommon* dx = nullptr;
        UnifiedPipeline* pipeline = nullptr;
        std::shared_ptr<ModelResource> resource;
        Vector4 baseColor = { 1, 1, 1, 1 };
        int32_t lightingMode = 1;
    };

    bool Initialize(const CreateInfo& ci);

    void SetWorld(const Matrix4x4& world) { world_ = world; }
    void SetColor(const Vector4& c);
    void SetLightingMode(int32_t m);
    void SetUVTransform(const Matrix4x4& uv);

    void Draw(const Matrix4x4& view, const Matrix4x4& proj, ID3D12Resource* directionalLightCB);

private:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D12Resource> CreateUploadBuffer_(size_t size);

    struct MaterialCB {
        Vector4 color{ 1, 1, 1, 1 };
        int32_t enableLighting = 1;
        float pad[3] = { 0, 0, 0 };
        Matrix4x4 uvTransform = MakeIdentity4x4();
    };

    struct TransformCB {
        Matrix4x4 WVP = MakeIdentity4x4();
        Matrix4x4 World = MakeIdentity4x4();
    };

private:
    DirectXCommon* dx_ = nullptr;
    UnifiedPipeline* pipeline_ = nullptr;
    std::shared_ptr<ModelResource> resource_;

    Matrix4x4 world_ = MakeIdentity4x4();

    ComPtr<ID3D12Resource> cbMaterial_;
    MaterialCB* cbMatMapped_ = nullptr;

    ComPtr<ID3D12Resource> cbTransform_;
    TransformCB* cbTransMapped_ = nullptr;
};
