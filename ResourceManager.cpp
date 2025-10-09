#include "ResourceManager.h"
#include <cassert>

using Microsoft::WRL::ComPtr;

void ResourceManager::Initialize(ID3D12Device *device) { device_ = device; }

ComPtr<ID3D12Resource>
ResourceManager::CreateUploadBuffer(size_t sizeInBytes) const {
  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = sizeInBytes;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ComPtr<ID3D12Resource> res;
  HRESULT hr = device_->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr, IID_PPV_ARGS(&res));
  if (FAILED(hr)) {
    return nullptr;
  }
  return res;
}

/* DescriptorHeap */
ComPtr<ID3D12DescriptorHeap>
ResourceManager::CreateDescriptorHeap(ID3D12Device *device,
                                      D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                      UINT numDescriptors, bool shaderVisible) {
  ComPtr<ID3D12DescriptorHeap> descriptorHeap;
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

/* TextureResource */
ComPtr<ID3D12Resource>
ResourceManager::CreateTextureResource(ID3D12Device *device,
                                       const DirectX::TexMetadata &metadata) {
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);
  resourceDesc.Height = UINT(metadata.height);
  resourceDesc.MipLevels = UINT16(metadata.mipLevels);
  resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
  resourceDesc.Format = metadata.format;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Dimension =
      static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

  ComPtr<ID3D12Resource> resource;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));
  return resource;
}

/* UploadTextureData */
void ResourceManager::UploadTextureData(
    ID3D12Resource *texture, const DirectX::ScratchImage &mipImages) {
  const DirectX::TexMetadata &md = mipImages.GetMetadata();
  for (size_t mip = 0; mip < md.mipLevels; ++mip) {
    const DirectX::Image *img = mipImages.GetImage(mip, 0, 0);
    HRESULT hr =
        texture->WriteToSubresource(UINT(mip), nullptr, img->pixels,
                                    UINT(img->rowPitch), UINT(img->slicePitch));
    assert(SUCCEEDED(hr));
  }
}

/* DepthStencil */
ComPtr<ID3D12Resource>
ResourceManager::CreateDepthStencilTextureResource(ID3D12Device *device,
                                                   INT32 width, INT32 height) {
  D3D12_RESOURCE_DESC desc{};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.DepthOrArraySize = 1;
  desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  desc.SampleDesc.Count = 1;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_CLEAR_VALUE clear{};
  clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  clear.DepthStencil.Depth = 1.0f;

  ComPtr<ID3D12Resource> res;
  HRESULT hr = device->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
      &clear, IID_PPV_ARGS(&res));
  assert(SUCCEEDED(hr));
  return res;
}

/* Handle helpers */
D3D12_CPU_DESCRIPTOR_HANDLE
ResourceManager::GetCPUDescriptorHandle(ID3D12DescriptorHeap *heap,
                                        uint32_t descriptorSize,
                                        uint32_t index) {
  auto h = heap->GetCPUDescriptorHandleForHeapStart();
  h.ptr += (descriptorSize * index);
  return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE
ResourceManager::GetGPUDescriptorHandle(ID3D12DescriptorHeap *heap,
                                        uint32_t descriptorSize,
                                        uint32_t index) {
  auto h = heap->GetGPUDescriptorHandleForHeapStart();
  h.ptr += (descriptorSize * index);
  return h;
}
