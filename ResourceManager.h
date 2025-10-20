#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include "TextureUtils.h" // DirectX::ScratchImage 等
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

class ResourceManager {
public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  // 初期化（SRVヒープ作成もここで行う）
  void Initialize(ID3D12Device *device, UINT descriptorCount = 256);

  // テクスチャのロード（キャッシュ付き）
  ID3D12Resource *LoadTexture(const std::string &filePath);

  // 任意のアップロードバッファ作成
  Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(size_t size);

  // SRV作成（外部から呼ぶ場合用）
  UINT CreateSRV(ID3D12Resource *resource,
                 const D3D12_SHADER_RESOURCE_VIEW_DESC *desc);

  // GPUハンドル取得
  D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const;

  // SRVヒープ取得
  ID3D12DescriptorHeap *GetSRVHeap() const { return srvHeap_.Get(); }

  std::pair<ID3D12Resource *, D3D12_GPU_DESCRIPTOR_HANDLE>
  LoadTextureWithHandle(const std::string &filePath);

private:
  struct TextureInfo {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    UINT srvIndex = 0;
  };

  ID3D12Device *device_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
  UINT descriptorSize_ = 0;
  UINT nextIndex_ = 1; // 0はImGui予約

  std::unordered_map<std::string, TextureInfo> textureCache_;
};
