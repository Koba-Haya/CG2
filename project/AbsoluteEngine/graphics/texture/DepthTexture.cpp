#include "DepthTexture.h"
#include "base/DirectXCommon.h"
#include <cassert>

void DepthTexture::Initialize(DirectXCommon* dx, uint32_t width, uint32_t height) {
    assert(dx);
    ID3D12Device* device = dx->GetDevice();

    // 1. リソース設定 (R24G8_TYPELESS)
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = width;
    resDesc.Height = height;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    resDesc.SampleDesc.Count = 1;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // 2. ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 3. クリア値設定
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // 4. リソース生成
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 初期状態は深度書き込み
        &clearValue,
        IID_PPV_ARGS(&resource_)
    );
    currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    assert(SUCCEEDED(hr));

    // 5. DSV作成
    dsvHandle_ = dx->AllocateDsv();
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(resource_.Get(), &dsvDesc, dsvHandle_);

    // 6. SRV作成
    SrvAllocator& srvAlloc = dx->GetSrvAllocator();
    uint32_t srvIndex = srvAlloc.Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    device->CreateShaderResourceView(resource_.Get(), &srvDesc, srvAlloc.Cpu(srvIndex));
    
    srvHandle_ = SrvHandle(&srvAlloc, srvIndex);
    srvGpuHandle_ = srvAlloc.Gpu(srvIndex);
}
