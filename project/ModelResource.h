#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <memory>

#include "ModelUtils.h"

class DirectXCommon;
class TextureResource;

// 共有GPUリソース（頂点バッファ + テクスチャ）
// 同じモデルを複数体描画するときに、ここを shared_ptr で共有する。
class ModelResource {
public:
    struct CreateInfo {
        DirectXCommon* dx = nullptr;
        ModelData modelData{};
        std::shared_ptr<TextureResource> texture; // nullptr の場合は modelData.material からロード
    };

    bool Initialize(const CreateInfo& ci);

    const D3D12_VERTEX_BUFFER_VIEW& GetVBV() const { return vbv_; }
    uint32_t GetVertexCount() const { return vertexCount_; }

    // Texture SRV の GPU handle（RootDescriptorTable に渡す用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandleGPU() const;

private:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D12Resource> CreateUploadBuffer_(DirectXCommon* dx, size_t size);

private:
    DirectXCommon* dx_ = nullptr;
    ComPtr<ID3D12Resource> vb_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    uint32_t vertexCount_ = 0;

    std::shared_ptr<TextureResource> texture_;
};
