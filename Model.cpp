#include "Model.h"
#include "ResourceManager.h"
#include "externals/DirectXTex/DirectXTex.h"

// 既存のヘルパ（あなたの環境にある実装を使用）
extern DirectX::ScratchImage LoadTexture(const std::string &filePath);

constexpr UINT Align256(UINT n) { return (n + 255) & ~255; }

// ===== SrvAllocator 実装（mainと同一の挙動） =====
void SrvAllocator::Init(ID3D12Device *dev, ID3D12DescriptorHeap *h) {
  device = dev;
  heap = h;
  inc = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  next = 1; // 0はImGui
}
UINT SrvAllocator::Allocate() { return next++; }
D3D12_CPU_DESCRIPTOR_HANDLE SrvAllocator::Cpu(UINT index) const {
  D3D12_CPU_DESCRIPTOR_HANDLE base = heap->GetCPUDescriptorHandleForHeapStart();
  base.ptr += size_t(index) * inc;
  return base;
}
D3D12_GPU_DESCRIPTOR_HANDLE SrvAllocator::Gpu(UINT index) const {
  D3D12_GPU_DESCRIPTOR_HANDLE base = heap->GetGPUDescriptorHandleForHeapStart();
  base.ptr += size_t(index) * inc;
  return base;
}

// ===== Model 実装 =====
bool Model::Initialize(const CreateInfo &ci) {
  assert(ci.dx && ci.pipeline && ci.srvAlloc);
  dx_ = ci.dx;
  pipeline_ = ci.pipeline;
  srvAlloc_ = ci.srvAlloc;

  ID3D12Device *device = dx_->GetDevice();
  ID3D12GraphicsCommandList *cmd = dx_->GetCommandList();

  // 頂点バッファ
  vb_ = CreateUploadBuffer(sizeof(VertexData) * ci.modelData.vertices.size());
  vbv_.BufferLocation = vb_->GetGPUVirtualAddress();
  vbv_.StrideInBytes = sizeof(VertexData);
  vbv_.SizeInBytes = UINT(sizeof(VertexData) * ci.modelData.vertices.size());
  {
    void *ptr = nullptr;
    vb_->Map(0, nullptr, &ptr);
    std::memcpy(ptr, ci.modelData.vertices.data(), vbv_.SizeInBytes);
  }

  // Material CB (PS: b0) — シェーダ側のレイアウトに一致
  // color / enableLighting / uvTransform。:contentReference[oaicite:4]{index=4}
  cbMaterial_ = CreateUploadBuffer(Align256(sizeof(Material)));
  cbMaterial_->Map(0, nullptr, reinterpret_cast<void **>(&cbMatMapped_));
  *cbMatMapped_ = {};
  cbMatMapped_->color = ci.baseColor;
  cbMatMapped_->enableLighting = ci.lightingMode;
  cbMatMapped_->uvTransform = MakeIdentity4x4();

  // Transform CB (VS: b0) — WVP & World。:contentReference[oaicite:5]{index=5}

  cbTransform_ = CreateUploadBuffer(Align256(sizeof(Transformation)));
  cbTransform_->Map(0, nullptr, reinterpret_cast<void **>(&cbTransMapped_));
  *cbTransMapped_ = {MakeIdentity4x4(), MakeIdentity4x4()};

  // Texture (PS: t0)
  if (!ci.modelData.material.textureFilePath.empty()) {
    CreateTextureFromFile(ci.modelData.material.textureFilePath);
  } else {
    // 空なら白1x1などを用意しても良い
  }

  return true;
}

void Model::SetColor(const Vector4 &color) { cbMatMapped_->color = color; }
void Model::SetLightingMode(int32_t mode) {
  cbMatMapped_->enableLighting =
      mode; // 0/1/2 に対応（PS分岐）。:contentReference[oaicite:6]{index=6}
}
void Model::SetUVTransform(const Matrix4x4 &uv) {
  cbMatMapped_->uvTransform = uv;
}
void Model::SetWorldTransform(const Matrix4x4 &world) { world_ = world; }

void Model::Draw(const Matrix4x4 &view, const Matrix4x4 &proj,
                 ID3D12Resource *directionalLightCB) {
  ID3D12GraphicsCommandList *cmd = dx_->GetCommandList();

  assert(tex_ != 0);      // GPUハンドルが空じゃない
  assert(dx_->GetSRVHeap() != nullptr); // ヒープがある

  // PSO / RootSig
  cmd->SetPipelineState(pipeline_->GetPipelineState());
  cmd->SetGraphicsRootSignature(pipeline_->GetRootSignature());

  // IA
  cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmd->IASetVertexBuffers(0, 1, &vbv_);

  // 変換行列（VS:b0）
  Matrix4x4 wvp = Multiply(world_, Multiply(view, proj));
  cbTransMapped_->WVP = wvp;
  cbTransMapped_->World = world_;
  cmd->SetGraphicsRootConstantBufferView(1,
                                         cbTransform_->GetGPUVirtualAddress());
  //           ↑ MakeObject3DDesc の “VS: b0” 構成準拠（RootParam順は PS:b0,
  //           VS:b0, SRV table, PS:b1）
  //             UnifiedPipeline
  //             で詰めた順序に合わせる。:contentReference[oaicite:7]{index=7}

  // マテリアル（PS:b0）
  cmd->SetGraphicsRootConstantBufferView(0,
                                         cbMaterial_->GetGPUVirtualAddress());

  // テクスチャ（PS: t0）
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmd->SetDescriptorHeaps(1, heaps);
  cmd->SetGraphicsRootDescriptorTable(2, texSrvHandleGPU_);

  // 平行光（PS:b1）
  if (directionalLightCB) {
    cmd->SetGraphicsRootConstantBufferView(
        3, directionalLightCB->GetGPUVirtualAddress());
  }

  // ビューポート/シザー（共通設定をそのまま利用）
  cmd->RSSetViewports(1, &dx_->GetViewport());
  cmd->RSSetScissorRects(1, &dx_->GetScissorRect());

  // Draw（インデックス無しの想定。必要ならIB追加して DrawIndexed）
  cmd->DrawInstanced(UINT(vbv_.SizeInBytes / vbv_.StrideInBytes), 1, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Model::CreateUploadBuffer(size_t size) {
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

void Model::CreateTextureFromFile(const std::string &path) {
  ID3D12Device *device = dx_->GetDevice();

  DirectX::ScratchImage mip = LoadTexture(path);
  const DirectX::TexMetadata &meta = mip.GetMetadata();
  tex_ = ResourceManager::CreateTextureResource(device, meta);
  ResourceManager::UploadTextureData(tex_.Get(), mip);

  // SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
  srv.Format = meta.format;
  srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv.Texture2D.MipLevels = UINT(meta.mipLevels);

  texSrvIndex_ = srvAlloc_->Allocate();
  device->CreateShaderResourceView(tex_.Get(), &srv,
                                   srvAlloc_->Cpu(texSrvIndex_));
  texSrvHandleGPU_ = srvAlloc_->Gpu(texSrvIndex_);
}
