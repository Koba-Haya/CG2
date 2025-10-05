// RenderTargets.cpp
#include "CommandSystem.h"
#include "DirectXDevice.h"
#include "RenderTarget.h"
#include "SwapChainManager.h"

bool RenderTarget::Initialize(DirectXDevice &dev, SwapChainManager &swap,
                               uint32_t width, uint32_t height) {
  // 1) RTV ヒープを作成（バックバッファ数ぶん）
  D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
  rtvDesc.NumDescriptors = kFrameCount;
  rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  if (FAILED(
          dev.Get()->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap_)))) {
    return false;
  }

  rtvIncSize_ = dev.RtvDescriptorSize();

  // 2) バックバッファから RTV 作成
  for (uint32_t i = 0; i < kFrameCount; ++i) {
    swap.Get()->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i]));
    D3D12_RENDER_TARGET_VIEW_DESC rtv{};
    rtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
        rtvHeap_->GetCPUDescriptorHandleForHeapStart(), i, rtvIncSize_);
    dev.Get()->CreateRenderTargetView(backbuffers_[i].Get(), &rtv, handle);
  }

  // 3) ビューポート/シザー（ウィンドウサイズ全域）
  viewport_.TopLeftX = 0.0f;
  viewport_.TopLeftY = 0.0f;
  viewport_.Width = static_cast<float>(width);
  viewport_.Height = static_cast<float>(height);
  viewport_.MinDepth = 0.0f;
  viewport_.MaxDepth = 1.0f;

  scissor_.left = 0;
  scissor_.top = 0;
  scissor_.right = static_cast<LONG>(width);
  scissor_.bottom = static_cast<LONG>(height);

  return true;
}

void RenderTarget::Release() {
  for (auto &bb : backbuffers_) {
    bb.Reset();
  }
  rtvHeap_.Reset();
}

void RenderTarget::BeginFrame(DirectXDevice &dev, CommandSystem &cmd,
                               SwapChainManager &swap,
                               const FLOAT clearColor[4],
                               D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) {
  auto *cl = cmd.BeginFrame(swap.CurrentBackBufferIndex());

  const uint32_t idx = swap.CurrentBackBufferIndex();

  // バックバッファを Present→RenderTarget に遷移
  auto toRT = CD3DX12_RESOURCE_BARRIER::Transition(
      backbuffers_[idx].Get(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  cl->ResourceBarrier(1, &toRT);

  // RTV/DSV をセット
  auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      rtvHeap_->GetCPUDescriptorHandleForHeapStart(), idx, rtvIncSize_);
  cl->OMSetRenderTargets(1, &rtv, FALSE, &dsvHandle);

  // クリア（色＋深度）
  cl->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
  cl->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                            nullptr);

  // VP/Scissor
  cl->RSSetViewports(1, &viewport_);
  cl->RSSetScissorRects(1, &scissor_);
}

void RenderTarget::EndFrame(DirectXDevice &dev, CommandSystem &cmd,
                             SwapChainManager &swap) {
  auto *cl = cmd.List();
  const uint32_t idx = swap.CurrentBackBufferIndex();

  // 1) RENDER_TARGET → PRESENT へ遷移
  D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      backbuffers_[idx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  cl->ResourceBarrier(1, &barrier);

  // 2) 送出（Execute & Signal）
  cmd.ExecuteAndSignal(idx);
}
