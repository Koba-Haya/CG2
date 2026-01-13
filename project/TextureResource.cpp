#include "TextureResource.h"

#include <Windows.h>
#include <string>
#include <filesystem>

#include "DirectXCommon.h"
#include "SrvAllocator.h"
#include "TextureUtils.h"
#include "externals/DirectXTex/DirectXTex.h"

static void TexResLog_(const std::string &msg) {
  OutputDebugStringA((msg + "\n").c_str());
}

bool TextureResource::CreateFromFile(DirectXCommon *dx,
                                     const std::string &filePath) {
  if (!dx) {
    TexResLog_("[TextureResource] dx is null");
    return false;
  }

  if (!std::filesystem::exists(filePath)) {
    TexResLog_(std::string("[TextureResource] file not found: ") + filePath);
    return false;
  }

  DirectX::ScratchImage mipImages = LoadTexture(filePath);
  const DirectX::TexMetadata &meta = mipImages.GetMetadata();
  return CreateFromMetadata(dx, mipImages, meta);
}

bool TextureResource::CreateFromMetadata(DirectXCommon *dx,
                                         const DirectX::ScratchImage &mipImages,
                                         const DirectX::TexMetadata &meta) {

  if (!dx) {
    TexResLog_("[TextureResource] dx is null");
    return false;
  }

  ID3D12Device *device = dx->GetDevice();
  if (!device) {
    TexResLog_("[TextureResource] device is null");
    return false;
  }

  texture_ = CreateTextureResource(device, meta);
  if (!texture_) {
    TexResLog_(
        "[TextureResource] CreateTextureResource failed (texture_ is null)");
    return false;
  }

  UploadTextureData(texture_, mipImages);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = meta.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);

  return CreateSrv_(dx, srvDesc);
}

bool TextureResource::CreateSrv_(DirectXCommon *dx,
                                 const D3D12_SHADER_RESOURCE_VIEW_DESC &desc) {
  if (!dx) {
    return false;
  }

  ID3D12Device *device = dx->GetDevice();
  if (!device) {
    return false;
  }

  if (!texture_) {
    TexResLog_("[TextureResource] texture_ is null when creating SRV");
    return false;
  }

  SrvAllocator &alloc = dx->GetSrvAllocator();
  const UINT index = alloc.Allocate();

  device->CreateShaderResourceView(texture_.Get(), &desc, alloc.Cpu(index));

  srv_ = SrvHandle(&alloc, index);
  srvGpu_ = alloc.Gpu(index);
  return true;
}
