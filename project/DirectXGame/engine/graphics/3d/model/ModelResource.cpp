#include "ModelResource.h"

#include <cassert>
#include <cstring>
#include <vector>

#include "DirectXCommon.h"
#include "ModelUtils.h"
#include "TextureManager.h"
#include "TextureResource.h"

bool ModelResource::Initialize(const CreateInfo &ci) {
  assert(ci.dx);
  assert(ci.modelData);

  dx_ = ci.dx;

  const std::vector<VertexData> vertices = FlattenVertices(*ci.modelData);

  vertexCount_ = static_cast<uint32_t>(vertices.size());
  const size_t vbSize = sizeof(VertexData) * vertices.size();

  vb_ = CreateUploadBuffer_(dx_, vbSize);

  vbv_.BufferLocation = vb_->GetGPUVirtualAddress();
  vbv_.StrideInBytes = sizeof(VertexData);
  vbv_.SizeInBytes = static_cast<UINT>(vbSize);

  if (vbSize > 0) {
    void *mapped = nullptr;
    HRESULT hr = vb_->Map(0, nullptr, &mapped);
    assert(SUCCEEDED(hr));
    std::memcpy(mapped, vertices.data(), vbSize);
    vb_->Unmap(0, nullptr);
  }

  if (ci.texture) {
    texture_ = ci.texture;
  } else {
    const std::string texPath = PickDiffuseTexturePath(*ci.modelData);
    if (!texPath.empty()) {
      texture_ = TextureManager::GetInstance()->Load(texPath);
    } else {
      texture_ = TextureManager::GetInstance()->Load("resources/white1x1.png");
    }
  }

  return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE ModelResource::GetTextureHandleGPU() const {
  if (!texture_) {
    return {};
  }
  return texture_->GetSrvGpu();
}

Microsoft::WRL::ComPtr<ID3D12Resource>
ModelResource::CreateUploadBuffer_(DirectXCommon *dx, size_t size) {
  assert(dx);

  auto device = dx->GetDevice();
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
