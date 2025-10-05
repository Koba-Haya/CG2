// SwapChainManager.cpp
#include "SwapChainManager.h"
#include "CommandSystem.h"
#include "DirectXDevice.h"
#include <cassert>

bool SwapChainManager::Initialize(DirectXDevice &dev, CommandSystem &cmd,
                                  HWND hwnd,
                                  uint32_t width, uint32_t height) {
  // 1) スワップチェーンを作成（Flip-Discard, ダブルバッファ）
  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = width;
  desc.Height = height;
  desc.Format = format_;
  desc.SampleDesc = {1, 0};
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = kFrameCount;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

  ComPtr<IDXGIFactory4> factory;
  dev.Adapter()->GetParent(IID_PPV_ARGS(&factory));

  ComPtr<IDXGISwapChain1> sc1;
  if (FAILED(factory->CreateSwapChainForHwnd(cmd.Queue(), hwnd, &desc, nullptr,
                                             nullptr, &sc1))) {
    return false;
  }
  ComPtr<IDXGISwapChain4> sc4;
  if (FAILED(sc1.As(&sc4))) {
    return false;
  }

  swapChain_ = sc4;
  return true;
}

void SwapChainManager::Resize(DirectXDevice &, CommandSystem &, uint32_t,
                              uint32_t) {
  // ※今回は最小実装のため未対応（必要になったら実装）
}

void SwapChainManager::Present(uint32_t syncInterval) {
  swapChain_->Present(syncInterval, 0);
}
