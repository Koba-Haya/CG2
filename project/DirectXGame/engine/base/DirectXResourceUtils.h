#pragma once
#include <d3d12.h>
#include <format>
#include <wrl.h>

// namespace省略
template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

// DescriptorHeap の共通ヘルパ
ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(const ComPtr<ID3D12Device> &device,
                     D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                     bool shaderVisible);

// DepthStencil 用テクスチャリソース作成
ComPtr<ID3D12Resource>
CreateDepthStencilTextureResource(const ComPtr<ID3D12Device> &device,
                                  INT32 width, INT32 height);

// 汎用 CPU / GPU ディスクリプタハンドル取得
D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
                       uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
                       uint32_t descriptorSize, uint32_t index);

// ★ 追加: UploadHeap バッファ作成
ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device> &device,
                                            size_t sizeInBytes);