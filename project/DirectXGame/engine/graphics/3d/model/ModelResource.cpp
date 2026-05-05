#include "ModelResource.h"
#include "ModelUtils.h"
#include "Renderer.h"
#include "TextureManager.h"
#include "TextureResource.h"
#include <cassert>
#include <d3d12.h>
#include <wrl/client.h>

struct ModelResource::Impl {
  Microsoft::WRL::ComPtr<ID3D12Resource> vb;
  D3D12_GPU_VIRTUAL_ADDRESS vbAddress = 0;
  unsigned int vbSize = 0;
  unsigned int vbStride = 0;
  uint32_t vertexCount = 0;
  
  Microsoft::WRL::ComPtr<ID3D12Resource> vbBone;
  D3D12_GPU_VIRTUAL_ADDRESS vbBoneAddress = 0;
  unsigned int vbBoneSize = 0;
  unsigned int vbBoneStride = 0;
  bool hasBones = false;
  
  std::shared_ptr<const ModelData> modelData;
  std::shared_ptr<TextureResource> texture;
};

ModelResource::ModelResource() : pImpl_(std::make_unique<Impl>()) {}
ModelResource::~ModelResource() = default;

bool ModelResource::Initialize(const CreateInfo &ci) {
  assert(ci.modelData);
  pImpl_->modelData = ci.modelData;
  auto *renderer = Renderer::GetInstance();

  const std::vector<VertexData> vertices = FlattenVertices(*ci.modelData);
  if (vertices.empty())
    return false;

  pImpl_->vertexCount = static_cast<uint32_t>(vertices.size());
  const size_t vbBufferSize = sizeof(VertexData) * vertices.size();

  pImpl_->vb = renderer->CreateUploadBuffer(vbBufferSize);
  if (!pImpl_->vb)
    return false;

  void *mapped = nullptr;
  if (SUCCEEDED(pImpl_->vb->Map(0, nullptr, &mapped))) {
    std::memcpy(mapped, vertices.data(), vbBufferSize);
    pImpl_->vb->Unmap(0, nullptr);
  }

  pImpl_->vbAddress = pImpl_->vb->GetGPUVirtualAddress();
  pImpl_->vbSize = static_cast<unsigned int>(vbBufferSize);
  pImpl_->vbStride = sizeof(VertexData);

  const std::vector<VertexBoneData> skinningData = FlattenSkinningData(*ci.modelData);
  if (!skinningData.empty() && skinningData.size() == vertices.size()) {
    pImpl_->hasBones = true;
    const size_t vbBoneBufferSize = sizeof(VertexBoneData) * skinningData.size();
    pImpl_->vbBone = renderer->CreateUploadBuffer(vbBoneBufferSize);
    if (pImpl_->vbBone) {
      void *boneMapped = nullptr;
      if (SUCCEEDED(pImpl_->vbBone->Map(0, nullptr, &boneMapped))) {
        std::memcpy(boneMapped, skinningData.data(), vbBoneBufferSize);
        pImpl_->vbBone->Unmap(0, nullptr);
      }
      pImpl_->vbBoneAddress = pImpl_->vbBone->GetGPUVirtualAddress();
      pImpl_->vbBoneSize = static_cast<unsigned int>(vbBoneBufferSize);
      pImpl_->vbBoneStride = sizeof(VertexBoneData);
    }
  }

  if (ci.texture) {
    pImpl_->texture = ci.texture;
  } else {
    const std::string texPath = PickDiffuseTexturePath(*ci.modelData);
    pImpl_->texture = TextureManager::GetInstance()->Load(
        texPath.empty() ? "resources/uvChecker.png" : texPath);
  }

  return true;
}

unsigned long long ModelResource::GetVBVAddress() const {
  return pImpl_->vbAddress;
}
unsigned int ModelResource::GetVBVSize() const { return pImpl_->vbSize; }
unsigned int ModelResource::GetVBVStride() const { return pImpl_->vbStride; }
uint32_t ModelResource::GetVertexCount() const { return pImpl_->vertexCount; }

const ModelData* ModelResource::GetModelData() const { return pImpl_->modelData.get(); }

bool ModelResource::HasBones() const { return pImpl_->hasBones; }
unsigned long long ModelResource::GetBoneVBVAddress() const { return pImpl_->vbBoneAddress; }
unsigned int ModelResource::GetBoneVBVSize() const { return pImpl_->vbBoneSize; }
unsigned int ModelResource::GetBoneVBVStride() const { return pImpl_->vbBoneStride; }

unsigned long long ModelResource::GetTextureHandleGPUAsUInt64() const {
  return pImpl_->texture ? pImpl_->texture->GetSrvGpu().ptr : 0;
}