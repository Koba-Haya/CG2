#pragma once
// DirectXTex（TextureUtilsで既に使ってる想定）
#include "externals/DirectXTex/DirectXTex.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

class ResourceManager {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  // ★ 追加: 初期化（デバイス保持）
  void Initialize(ID3D12Device *device);

  // ★ 追加: UploadHeap のバッファ生成（汎用）
  ComPtr<ID3D12Resource> CreateUploadBuffer(size_t sizeInBytes) const;

  // ===== DescriptorHeap =====
  static ComPtr<ID3D12DescriptorHeap>
  CreateDescriptorHeap(ID3D12Device *device,
                       D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                       bool shaderVisible);

  // ===== Texture =====
  static ComPtr<ID3D12Resource>
  CreateTextureResource(ID3D12Device *device,
                        const DirectX::TexMetadata &metadata);

  static void UploadTextureData(ID3D12Resource *texture,
                                const DirectX::ScratchImage &mipImages);

  // ===== DepthStencil =====
  static ComPtr<ID3D12Resource>
  CreateDepthStencilTextureResource(ID3D12Device *device, INT32 width,
                                    INT32 height);

  // ===== Descriptor Handle helper =====
  static D3D12_CPU_DESCRIPTOR_HANDLE
  GetCPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap,
                         uint32_t descriptorSize, uint32_t index);

  static D3D12_GPU_DESCRIPTOR_HANDLE
  GetGPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap,
                         uint32_t descriptorSize, uint32_t index);

private:
  ID3D12Device *device_ = nullptr;
};
