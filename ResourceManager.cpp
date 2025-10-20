#include "ResourceManager.h"
#include <cassert>
#include <filesystem>

using Microsoft::WRL::ComPtr;
namespace fs = std::filesystem;

void ResourceManager::Initialize(ID3D12Device *device, UINT descriptorCount) {
  device_ = device;
  descriptorSize_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // SRV用ディスクリプタヒープを作成（ImGuiと共用する前提）
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.NumDescriptors = descriptorCount;
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT hr = device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
  assert(SUCCEEDED(hr));
  nextIndex_ = 1; // 0はImGui用に予約
}

ID3D12Resource *ResourceManager::LoadTexture(const std::string &filePath) {
  // キャッシュチェック
  if (textureCache_.contains(filePath)) {
    return textureCache_[filePath].resource.Get();
  }

  assert(device_);
  assert(fs::exists(filePath));

  // テクスチャ読み込み
  DirectX::ScratchImage mipImages{};
  HRESULT hr = DirectX::LoadFromWICFile(
      std::wstring(filePath.begin(), filePath.end()).c_str(),
      DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, mipImages);
  assert(SUCCEEDED(hr));

  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();

  // テクスチャリソース作成
  ComPtr<ID3D12Resource> texture =
      TextureUtils::CreateTextureResource(device_, metadata);
  TextureUtils::UploadTextureData(texture.Get(), mipImages);

  // SRV作成
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  UINT index = CreateSRV(texture.Get(), &srvDesc);

  // キャッシュ登録
  textureCache_[filePath] = {texture, index};
  return texture.Get();
}

Microsoft::WRL::ComPtr<ID3D12Resource>
ResourceManager::CreateUploadBuffer(size_t size) {
  D3D12_HEAP_PROPERTIES heapProp{};
  heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resDesc{};
  resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resDesc.Width = size;
  resDesc.Height = 1;
  resDesc.DepthOrArraySize = 1;
  resDesc.MipLevels = 1;
  resDesc.SampleDesc.Count = 1;
  resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ComPtr<ID3D12Resource> buffer;
  HRESULT hr = device_->CreateCommittedResource(
      &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
  assert(SUCCEEDED(hr));
  return buffer;
}

UINT ResourceManager::CreateSRV(ID3D12Resource *resource,
                                const D3D12_SHADER_RESOURCE_VIEW_DESC *desc) {
  assert(device_ && srvHeap_);
  UINT index = nextIndex_++;

  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      srvHeap_->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += index * descriptorSize_;
  device_->CreateShaderResourceView(resource, desc, handleCPU);
  return index;
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::GetGPUHandle(UINT index) const {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      srvHeap_->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += index * descriptorSize_;
  return handleGPU;
}
