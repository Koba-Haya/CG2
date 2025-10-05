#include "DepthStencil.h"
#include "DirectXDevice.h"

bool DepthStencil::Initialize(DirectXDevice &dev, UINT width, UINT height) {
  device_ = dev.Get();

  // 1) DSV用ヒープ(1スロット)
  D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
  dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvDesc.NumDescriptors = 1;
  dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  if (FAILED(device_->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap_))))
    return false;

  // 2) 深度リソース作成
  //    クリア値を持たせる（ClearDepthStencilViewで使う）
  D3D12_CLEAR_VALUE clearValue{};
  clearValue.Format = DXGI_FORMAT_D32_FLOAT;
  clearValue.DepthStencil.Depth = 1.0f;
  clearValue.DepthStencil.Stencil = 0;

  D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
      DXGI_FORMAT_D32_FLOAT, width, height, /*arraySize=*/1, /*mipLevels=*/1);
  texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  if (FAILED(device_->CreateCommittedResource(
          &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
          D3D12_RESOURCE_STATE_DEPTH_WRITE, // 初期状態は書き込み
          &clearValue, IID_PPV_ARGS(&depthResource_)))) {
    return false;
  }

  // 3) DSV を作成してヒープに置く
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
  dsv.Format = DXGI_FORMAT_D32_FLOAT;
  dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsv.Flags = D3D12_DSV_FLAG_NONE;

  device_->CreateDepthStencilView(
      depthResource_.Get(), &dsv,
      dsvHeap_->GetCPUDescriptorHandleForHeapStart());
  return true;
}