#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <string>
#include <vector>
#include <wrl.h>

// namespace省略
template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

std::wstring ConvertString(const std::string &str);
std::string ConvertString(const std::wstring &str);

DirectX::ScratchImage LoadTexture(const std::string &filePath);

ComPtr<ID3D12Resource>
CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                      const DirectX::TexMetadata &metadata);

// void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
//                        const DirectX::ScratchImage &mipImages);

ComPtr<ID3D12Resource>
UploadTextureData(const ComPtr<ID3D12Resource> &texture,
                  const DirectX::ScratchImage &mipImages,
                  const ComPtr<ID3D12Device> &device,
                  const ComPtr<ID3D12GraphicsCommandList> &commandList);