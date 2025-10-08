#include "Sprite.h"
#include "TextureUtils.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <cassert>
#include <string>

struct SpriteVertex {
  Vector3 pos; // POSITION
  Vector2 uv;  // TEXCOORD
};

struct SpriteMaterial {
  Vector4 color;
  int enableLighting; // 0:Unlit
  float pad[3];       // 16byte アライン用
  Matrix4x4 uvTransform;
};

struct SpriteTransform {
  Matrix4x4 WVP;
  Matrix4x4 World;
};

constexpr UINT Align256(UINT n) { return (n + 255) & ~255; }

// ✅ テクスチャリソース作成
Microsoft::WRL::ComPtr<ID3D12Resource>
CreateTextureResource(ID3D12Device *device,
                      const DirectX::TexMetadata &metadata) {
  D3D12_RESOURCE_DESC desc{};
  desc.Width = static_cast<UINT>(metadata.width);
  desc.Height = static_cast<UINT>(metadata.height);
  desc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
  desc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
  desc.Format = metadata.format;
  desc.SampleDesc.Count = 1;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  // ←★ WriteBack + GenericRead で作る
  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_CUSTOM;
  heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heap.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

  Microsoft::WRL::ComPtr<ID3D12Resource> tex;
  HRESULT hr = device->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, // ← SRVにそのまま使える
      nullptr, IID_PPV_ARGS(&tex));
  assert(SUCCEEDED(hr));
  return tex;
}

// GPUバッファ（頂点・インデックス・CB用）を作る汎用関数
Microsoft::WRL::ComPtr<ID3D12Resource>
CreateBufferResource(ID3D12Device *device, size_t sizeInBytes) {

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = sizeInBytes;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
  HRESULT hr = device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
  assert(SUCCEEDED(hr));
  return buffer;
}

// ✅ 仮の UploadTextureData（実際の転送は未実装でもOK）
void UploadTextureData(ID3D12Resource *tex,
                       const DirectX::ScratchImage &mipImages) {
  const DirectX::TexMetadata &meta = mipImages.GetMetadata();
  for (size_t mip = 0; mip < meta.mipLevels; ++mip) {
    const DirectX::Image *img = mipImages.GetImage(mip, 0, 0);
    // WriteBack ヒープなのでこれで直接書ける
    HRESULT hr = tex->WriteToSubresource(
        static_cast<UINT>(mip), nullptr, img->pixels,
        static_cast<UINT>(img->rowPitch), static_cast<UINT>(img->slicePitch));
    assert(SUCCEEDED(hr));
  }
}

bool Sprite::Initialize(const CreateInfo &info) {
  dx_ = info.dx;
  pipeline_ = info.pipeline;
  srvAlloc_ = info.srvAlloc;
  color_ = info.color;

  // テクスチャ読み込み
  DirectX::ScratchImage mipImages = LoadTexture(info.texturePath);
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  texture_ = CreateTextureResource(dx_->GetDevice(), metadata);
  UploadTextureData(texture_.Get(), mipImages);

  // SRVを作成
  UINT texIdx = srvAlloc_->Allocate();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);
  dx_->GetDevice()->CreateShaderResourceView(texture_.Get(), &srvDesc,
                                             srvAlloc_->Cpu(texIdx));
  textureHandle_ = srvAlloc_->Gpu(texIdx);

  // 頂点バッファ
  SpriteVertex vertices[4] = {
      {{0, info.size.y, 0}, {0, 1}},
      {{0, 0, 0}, {0, 0}},
      {{info.size.x, info.size.y, 0}, {1, 1}},
      {{info.size.x, 0, 0}, {1, 0}},
  };

  vertexBuffer_ = CreateBufferResource(dx_->GetDevice(), sizeof(vertices));
  void *vData = nullptr;
  vertexBuffer_->Map(0, nullptr, &vData);
  memcpy(vData, vertices, sizeof(vertices));
  vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
  vbView_.SizeInBytes = sizeof(vertices);
  vbView_.StrideInBytes = sizeof(SpriteVertex);

  // インデックスバッファ
  uint32_t indices[6] = {0, 1, 2, 1, 3, 2};
  indexBuffer_ = CreateBufferResource(dx_->GetDevice(), sizeof(indices));
  uint32_t *iData = nullptr;
  indexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&iData));
  memcpy(iData, indices, sizeof(indices));
  ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
  ibView_.SizeInBytes = sizeof(indices);
  ibView_.Format = DXGI_FORMAT_R32_UINT;

  // マテリアル
  materialBuffer_ =
      CreateBufferResource(dx_->GetDevice(), Align256(sizeof(SpriteMaterial)));
  SpriteMaterial *mData = nullptr;
  materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mData));
  mData->color = color_;
  mData->enableLighting = 0; // スプライトはアンリット
  mData->pad[0] = 0;
  mData->pad[1] = 0;
  mData->pad[2] = 0;
  mData->uvTransform = uvMatrix_;

  // Transform
  transformBuffer_ =
      CreateBufferResource(dx_->GetDevice(), Align256(sizeof(SpriteTransform)));
  SpriteTransform *tData = nullptr;
  transformBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&tData));
  tData->WVP = MakeIdentity4x4();
  tData->World = worldMatrix_; 

  return true;
}

void Sprite::SetPosition(const Vector3 &pos) { position_ = pos; }
void Sprite::SetScale(const Vector3 &scale) { scale_ = scale; }
void Sprite::SetRotation(const Vector3 &rot) { rotation_ = rot; }
void Sprite::SetUVTransform(const Matrix4x4 &uv) { uvMatrix_ = uv; }
void Sprite::SetColor(const Vector4 &color) { color_ = color; }

void Sprite::Draw(const Matrix4x4 &view, const Matrix4x4 &proj) {
  worldMatrix_ = MakeAffineMatrix(scale_, rotation_, position_);
  Matrix4x4 wvp = Multiply(worldMatrix_, Multiply(view, proj));
  {
    SpriteTransform *tData = nullptr;
    transformBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&tData));
    tData->WVP = wvp;
    tData->World = worldMatrix_;
  }

  {
    SpriteMaterial *mData = nullptr;
    materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mData));
    mData->color = color_;
    mData->uvTransform = uvMatrix_;
  }

  ID3D12GraphicsCommandList *cmd = dx_->GetCommandList();
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmd->SetDescriptorHeaps(1, heaps);
  cmd->SetGraphicsRootSignature(pipeline_->GetRootSignature());
  cmd->SetPipelineState(pipeline_->GetPipelineState());
  cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmd->IASetVertexBuffers(0, 1, &vbView_);
  cmd->IASetIndexBuffer(&ibView_);

  cmd->SetGraphicsRootConstantBufferView(
      0, materialBuffer_->GetGPUVirtualAddress()); // PS:b0
  cmd->SetGraphicsRootConstantBufferView(
      1, transformBuffer_->GetGPUVirtualAddress()); // VS:b0
  cmd->SetGraphicsRootDescriptorTable(2, textureHandle_);

  cmd->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
