#include "ModelInstance.h"

#include <cassert>

#include "DirectXCommon.h"
#include "ModelResource.h"
#include "UnifiedPipeline.h"
#include "Renderer.h"
#include "UnifiedPipeline.h"

static constexpr UINT Align256_(UINT n) { return (n + 255) & ~255u; }

bool ModelInstance::Initialize(const CreateInfo &ci) {
  dx_ = Renderer::GetInstance()->GetDX();
  assert(dx_ && ci.resource);

  resource_ = ci.resource;

  world_ = MakeIdentity4x4();

  cbMaterial_ = CreateUploadBuffer_(Align256_(sizeof(MaterialCB)));
  HRESULT hr =
      cbMaterial_->Map(0, nullptr, reinterpret_cast<void **>(&cbMatMapped_));
  assert(SUCCEEDED(hr));
  assert(cbMatMapped_);

  *cbMatMapped_ = {};
  cbMatMapped_->color = ci.baseColor;
  cbMatMapped_->enableLighting = ci.lightingMode;
  cbMatMapped_->specularColor = ci.specularColor;
  cbMatMapped_->uvTransform = MakeIdentity4x4();
  cbMatMapped_->shininess = ci.shininess;

  cbTransform_ = CreateUploadBuffer_(Align256_(sizeof(TransformCB)));
  hr =
      cbTransform_->Map(0, nullptr, reinterpret_cast<void **>(&cbTransMapped_));
  assert(SUCCEEDED(hr));
  assert(cbTransMapped_);

  *cbTransMapped_ = {MakeIdentity4x4(), MakeIdentity4x4(), MakeIdentity4x4()};

  return true;
}

void ModelInstance::SetColor(const Vector4 &c) {
  assert(cbMatMapped_);
  cbMatMapped_->color = c;
}

void ModelInstance::SetLightingMode(int32_t m) {
  assert(cbMatMapped_);
  cbMatMapped_->enableLighting = m;
}

void ModelInstance::SetUVTransform(const Matrix4x4 &uv) {
  assert(cbMatMapped_);
  cbMatMapped_->uvTransform = uv;
}

void ModelInstance::SetSpecularColor(const Vector3 &c) {
  assert(cbMatMapped_);
  cbMatMapped_->specularColor = c;
}

void ModelInstance::SetShininess(float s) {
  assert(cbMatMapped_);
  cbMatMapped_->shininess = s;
}

void ModelInstance::Draw() {
  Renderer::GetInstance()->DrawModel(this);
}

void ModelInstance::DrawInternal(ID3D12GraphicsCommandList *cmd, const Matrix4x4 &view, const Matrix4x4 &proj) {
  assert(dx_ && resource_);
  assert(cbMatMapped_ && cbTransMapped_);

  cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  const auto &vbv = resource_->GetVBV();
  cmd->IASetVertexBuffers(0, 1, &vbv);

  const Matrix4x4 wvp = Multiply(world_, Multiply(view, proj));
  cbTransMapped_->WVP = wvp;
  cbTransMapped_->World = world_;
  cbTransMapped_->WorldInverseTranspose = Transpose(Inverse(world_));

  // Root layout (MakeObject3DDesc 想定)
  // 0: PS b0 Material
  cmd->SetGraphicsRootConstantBufferView(0,
                                         cbMaterial_->GetGPUVirtualAddress());

  // 1: VS b0 Transform
  cmd->SetGraphicsRootConstantBufferView(1,
                                         cbTransform_->GetGPUVirtualAddress());

  // 2: PS t0 Texture table
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmd->SetDescriptorHeaps(1, heaps);
  cmd->SetGraphicsRootDescriptorTable(2, resource_->GetTextureHandleGPU());

  // 3-6: DirLight, Camera, PointLight, SpotLight are set by Renderer


  cmd->RSSetViewports(1, &dx_->GetViewport());
  cmd->RSSetScissorRects(1, &dx_->GetScissorRect());

  cmd->DrawInstanced(resource_->GetVertexCount(), 1, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource>
ModelInstance::CreateUploadBuffer_(size_t size) {
  assert(dx_);
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
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
  assert(SUCCEEDED(hr));
  return res;
}
