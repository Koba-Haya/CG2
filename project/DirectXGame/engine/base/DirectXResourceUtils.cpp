#include "DirectXResourceUtils.h"
#include <cassert>

ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(const ComPtr<ID3D12Device> &device,
                     D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                     bool shaderVisible) {
  ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = heapType;
  desc.NumDescriptors = numDescriptors;
  desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                             : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr =
      device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
  assert(SUCCEEDED(hr));
  return descriptorHeap;
}

ComPtr<ID3D12Resource>
CreateDepthStencilTextureResource(const ComPtr<ID3D12Device> &device,
                                  INT32 width, INT32 height) {
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = width;
  resourceDesc.Height = height;
  resourceDesc.MipLevels = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_CLEAR_VALUE depthClear{};
  depthClear.DepthStencil.Depth = 1.0f;
  depthClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

  ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));
  return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
                       uint32_t descriptorSize, uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE h =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  h.ptr += size_t(descriptorSize) * index;
  return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
                       uint32_t descriptorSize, uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE h =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();
  h.ptr += size_t(descriptorSize) * index;
  return h;
}

// ★ 追加実装
ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device> &device,
                                            size_t sizeInBytes) {
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = sizeInBytes;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ComPtr<ID3D12Resource> res;
  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
  assert(SUCCEEDED(hr));
  return res;
}