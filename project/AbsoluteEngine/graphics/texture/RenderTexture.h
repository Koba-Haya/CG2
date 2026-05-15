#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "Vector.h"
#include "SrvHandle.h"

class DirectXCommon;

// オフスクリーンレンダリング用のテクスチャリソースを管理するクラス
class RenderTexture {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    RenderTexture() = default;
    ~RenderTexture() = default;

    // 初期化
    void Initialize(DirectXCommon* dx, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);

    // ゲッター
    ID3D12Resource* GetResource() const { return resource_.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return rtvHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; }

private:
    ComPtr<ID3D12Resource> resource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};
    
    SrvHandle srvHandle_;
};
