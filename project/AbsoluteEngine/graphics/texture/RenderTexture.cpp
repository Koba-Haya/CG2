#include "RenderTexture.h"
#include "DirectXCommon.h"
#include <cassert>

void RenderTexture::Initialize(DirectXCommon* dx, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor) {
    assert(dx);
    ID3D12Device* device = dx->GetDevice();

    // 1. リソース設定
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = width;
    resDesc.Height = height;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = format;
    resDesc.SampleDesc.Count = 1;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // 2. ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 3. クリア値設定
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = format;
    clearValue.Color[0] = clearColor.x;
    clearValue.Color[1] = clearColor.y;
    clearValue.Color[2] = clearColor.z;
    clearValue.Color[3] = clearColor.w;

    // 4. リソース生成
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&resource_)
    );
    assert(SUCCEEDED(hr));

    // 5. RTV作成
    rtvHandle_ = dx->AllocateRtv();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);

    // 6. SRV作成
    SrvAllocator& srvAlloc = dx->GetSrvAllocator();
    uint32_t srvIndex = srvAlloc.Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    device->CreateShaderResourceView(resource_.Get(), &srvDesc, srvAlloc.Cpu(srvIndex));
    
    srvHandle_ = SrvHandle(&srvAlloc, srvIndex);
    srvGpuHandle_ = srvAlloc.Gpu(srvIndex);
}
