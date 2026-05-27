#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "SrvHandle.h"

class DirectXCommon;

// オフスクリーンレンダリング用の深度テクスチャリソースを管理するクラス
class DepthTexture {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    DepthTexture() = default;
    ~DepthTexture() = default;

    // 初期化
    void Initialize(DirectXCommon* dx, uint32_t width, uint32_t height);

    // ゲッター
    ID3D12Resource* GetResource() const { return resource_.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const { return dsvHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; }

    D3D12_RESOURCE_STATES GetCurrentState() const { return currentState_; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { currentState_ = state; }

private:
    ComPtr<ID3D12Resource> resource_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};
    
    D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    SrvHandle srvHandle_;
};
