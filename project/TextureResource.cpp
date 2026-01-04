#include "TextureResource.h"

#include <cassert>

#include "DirectXCommon.h"
#include "SrvAllocator.h"
#include "TextureUtils.h"

#include "externals/DirectXTex/DirectXTex.h"

bool TextureResource::CreateFromFile(DirectXCommon* dx, const std::string& filePath) {
    assert(dx);

    DirectX::ScratchImage mipImages = LoadTexture(filePath);
    const DirectX::TexMetadata& meta = mipImages.GetMetadata();
    return CreateFromMetadata(dx, mipImages, meta);
}

bool TextureResource::CreateFromMetadata(DirectXCommon* dx,
    const DirectX::ScratchImage& mipImages,
    const DirectX::TexMetadata& meta) {
    assert(dx);

    ID3D12Device* device = dx->GetDevice();

    texture_ = CreateTextureResource(device, meta);
    UploadTextureData(texture_, mipImages);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = meta.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);

    return CreateSrv_(dx, srvDesc);
}

bool TextureResource::CreateSrv_(DirectXCommon* dx, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
    assert(dx);

    ID3D12Device* device = dx->GetDevice();
    SrvAllocator& alloc = dx->GetSrvAllocator();

    const UINT index = alloc.Allocate();
    device->CreateShaderResourceView(texture_.Get(), &desc, alloc.Cpu(index));

    srv_ = SrvHandle(&alloc, index);
    srvGpu_ = alloc.Gpu(index);
    return true;
}
