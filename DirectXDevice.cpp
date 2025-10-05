// DxDevice.cpp
#include "DirectXDevice.h"
#include <cassert>

bool DirectXDevice::Initialize(bool withDebug) {
  // 1) デバッグレイヤ（任意）
  debugEnabled_ = withDebug;
#if defined(_DEBUG)
  if (withDebug) {
    ComPtr<ID3D12Debug1> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
      debug->EnableDebugLayer();
      debug->SetEnableGPUBasedValidation(FALSE);
    }
  }
#endif

  // 2) DXGI Factory 生成
  UINT flags = 0;
#if defined(_DEBUG)
  if (withDebug) {
    flags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif
  HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory_));
  if (FAILED(hr)) {
    return false;
  }

  // 3) ハードウェアアダプタの選択
  if (!CreateAdapter_()) {
    return false;
  }

  // 4) Device 生成
  if (!CreateDevice_()) {
    return false;
  }

  // 5) ディスクリプタサイズを保存（ヒープ管理で利用）
  rtvIncSize_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  dsvIncSize_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  cbvSrvUavIncSize_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  return true;
}

bool DirectXDevice::CreateAdapter_() {
  // 高性能優先でアダプタ列挙
  ComPtr<IDXGIFactory6> factory6;
  if (FAILED(factory_.As(&factory6))) {
    return false;
  }

  for (UINT index = 0;; ++index) {
    ComPtr<IDXGIAdapter1> tmp;
    if (factory6->EnumAdapterByGpuPreference(
            index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmp)) ==
        DXGI_ERROR_NOT_FOUND) {
      break;
    }
    DXGI_ADAPTER_DESC1 desc{};
    tmp->GetDesc1(&desc);
    // ソフトウェアアダプタは除外
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    // D3D12 が作れるかどうかを事前にチェック
    if (SUCCEEDED(D3D12CreateDevice(tmp.Get(), D3D_FEATURE_LEVEL_11_0,
                                    _uuidof(ID3D12Device), nullptr))) {
      // Adapter4 にクエリ
      if (SUCCEEDED(tmp.As(&adapter_))) {
        return true;
      }
    }
  }
  return false;
}

bool DirectXDevice::CreateDevice_() {
  if (!adapter_) {
    return false;
  }
  HRESULT hr = D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(&device_));
  return SUCCEEDED(hr);
}
