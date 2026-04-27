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
  std::shared_ptr<TextureResource> texture;
};

ModelResource::ModelResource() : pImpl_(std::make_unique<Impl>()) {}
ModelResource::~ModelResource() = default;

bool ModelResource::Initialize(const CreateInfo &ci) {
  assert(ci.modelData);
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

  if (ci.texture) {
    pImpl_->texture = ci.texture;
  } else {
    const std::string texPath = PickDiffuseTexturePath(*ci.modelData);
    pImpl_->texture = TextureManager::GetInstance()->Load(
        texPath.empty() ? "resources/white1x1.png" : texPath);
  }

  return true;
}

unsigned long long ModelResource::GetVBVAddress() const {
  return pImpl_->vbAddress;
}
unsigned int ModelResource::GetVBVSize() const { return pImpl_->vbSize; }
unsigned int ModelResource::GetVBVStride() const { return pImpl_->vbStride; }
uint32_t ModelResource::GetVertexCount() const { return pImpl_->vertexCount; }

unsigned long long ModelResource::GetTextureHandleGPUAsUInt64() const {
  return pImpl_->texture ? pImpl_->texture->GetSrvGpu().ptr : 0;
}