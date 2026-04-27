#include "SpriteResource.h"
#include "Renderer.h"
#include "TextureManager.h"
#include "TextureResource.h"
#include <cassert>
#include <d3d12.h>
#include <wrl/client.h>

struct SpriteResource::Impl {
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
  std::shared_ptr<TextureResource> texture;
};

SpriteResource::SpriteResource() : pImpl_(std::make_unique<Impl>()) {}
SpriteResource::~SpriteResource() = default;

bool SpriteResource::Initialize(const std::string &texturePath) {
  auto *renderer = Renderer::GetInstance();

  pImpl_->texture = TextureManager::GetInstance()->Load(texturePath);
  if (!pImpl_->texture)
    return false;

  SpriteVertex vertices[4] = {
      {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  };
  uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

  pImpl_->vertexBuffer = renderer->CreateUploadBuffer(sizeof(vertices));
  void *vbData = nullptr;
  pImpl_->vertexBuffer->Map(0, nullptr, &vbData);
  std::memcpy(vbData, vertices, sizeof(vertices));
  pImpl_->vertexBuffer->Unmap(0, nullptr);

  pImpl_->indexBuffer = renderer->CreateUploadBuffer(sizeof(indices));
  void *ibData = nullptr;
  pImpl_->indexBuffer->Map(0, nullptr, &ibData);
  std::memcpy(ibData, indices, sizeof(indices));
  pImpl_->indexBuffer->Unmap(0, nullptr);

  return true;
}

unsigned long long SpriteResource::GetVBAddress() const {
  return pImpl_->vertexBuffer->GetGPUVirtualAddress();
}
unsigned int SpriteResource::GetVBSize() const {
  return (unsigned int)pImpl_->vertexBuffer->GetDesc().Width;
}
unsigned long long SpriteResource::GetIBAddress() const {
  return pImpl_->indexBuffer->GetGPUVirtualAddress();
}
unsigned int SpriteResource::GetIBSize() const {
  return (unsigned int)pImpl_->indexBuffer->GetDesc().Width;
}
std::shared_ptr<TextureResource> SpriteResource::GetTexture() const {
  return pImpl_->texture;
}