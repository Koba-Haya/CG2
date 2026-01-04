#include "ModelInstance.h"

#include <cassert>

#include "DirectXCommon.h"
#include "ModelResource.h"
#include "UnifiedPipeline.h"

constexpr UINT Align256(UINT n) { return (n + 255) & ~255; }

bool ModelInstance::Initialize(const CreateInfo& ci) {
    assert(ci.dx && ci.pipeline && ci.resource);
    dx_ = ci.dx;
    pipeline_ = ci.pipeline;
    resource_ = ci.resource;

    world_ = MakeIdentity4x4();

    cbMaterial_ = CreateUploadBuffer_(Align256(sizeof(MaterialCB)));
    cbMaterial_->Map(0, nullptr, reinterpret_cast<void**>(&cbMatMapped_));
    *cbMatMapped_ = {};
    cbMatMapped_->color = ci.baseColor;
    cbMatMapped_->enableLighting = ci.lightingMode;
    cbMatMapped_->uvTransform = MakeIdentity4x4();

    cbTransform_ = CreateUploadBuffer_(Align256(sizeof(TransformCB)));
    cbTransform_->Map(0, nullptr, reinterpret_cast<void**>(&cbTransMapped_));
    *cbTransMapped_ = { MakeIdentity4x4(), MakeIdentity4x4() };

    return true;
}

void ModelInstance::SetColor(const Vector4& c) { cbMatMapped_->color = c; }
void ModelInstance::SetLightingMode(int32_t m) { cbMatMapped_->enableLighting = m; }
void ModelInstance::SetUVTransform(const Matrix4x4& uv) { cbMatMapped_->uvTransform = uv; }

void ModelInstance::Draw(const Matrix4x4& view, const Matrix4x4& proj, ID3D12Resource* directionalLightCB) {
    assert(dx_ && pipeline_ && resource_);

    ID3D12GraphicsCommandList* cmd = dx_->GetCommandList();

    cmd->SetPipelineState(pipeline_->GetPipelineState());
    cmd->SetGraphicsRootSignature(pipeline_->GetRootSignature());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    const auto& vbv = resource_->GetVBV();
    cmd->IASetVertexBuffers(0, 1, &vbv);

    // VS:b0
    Matrix4x4 wvp = Multiply(world_, Multiply(view, proj));
    cbTransMapped_->WVP = wvp;
    cbTransMapped_->World = world_;
    cmd->SetGraphicsRootConstantBufferView(1, cbTransform_->GetGPUVirtualAddress());

    // PS:b0
    cmd->SetGraphicsRootConstantBufferView(0, cbMaterial_->GetGPUVirtualAddress());

    // SRV table (PS:t0)
    ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
    cmd->SetDescriptorHeaps(1, heaps);
    cmd->SetGraphicsRootDescriptorTable(2, resource_->GetTextureHandleGPU());

    // PS:b1 (DirectionalLight)
    if (directionalLightCB) {
        cmd->SetGraphicsRootConstantBufferView(3, directionalLightCB->GetGPUVirtualAddress());
    }

    cmd->RSSetViewports(1, &dx_->GetViewport());
    cmd->RSSetScissorRects(1, &dx_->GetScissorRect());

    cmd->DrawInstanced(resource_->GetVertexCount(), 1, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource> ModelInstance::CreateUploadBuffer_(size_t size) {
    auto device = dx_->GetDevice();
    ComPtr<ID3D12Resource> res;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&res));
    assert(SUCCEEDED(hr));
    return res;
}
