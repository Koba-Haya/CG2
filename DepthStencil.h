#pragma once
#include "externals/DirectXTex/d3dx12.h"
#include <d3d12.h>
#include <wrl/client.h>

class DirectXDevice;

/**
 * 深度ステンシル用の2Dテクスチャを1枚だけ作る最小クラス。
 * - Defaultヒープ上に D32_FLOAT で確保
 * - DSVヒープ(1スロット)を持つ
 * - BeginFrameでのクリア用に DSV の CPUハンドルを外に渡す
 */
class DepthStencil {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  bool Initialize(DirectXDevice &dev, UINT width, UINT height);

  // DSVのCPUハンドルを返す（OMSetRenderTargets/クリアで使用）
  D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle() const {
    return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
  }

  void Release() {
    depthResource_.Reset();
    dsvHeap_.Reset();
    device_ = nullptr;
  }

private:
  ID3D12Device *device_ = nullptr;
  ComPtr<ID3D12Resource> depthResource_;
  ComPtr<ID3D12DescriptorHeap> dsvHeap_;
};
