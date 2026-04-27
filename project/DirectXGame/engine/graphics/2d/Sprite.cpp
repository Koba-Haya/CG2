#include "Sprite.h"
#include "Renderer.h"
#include "SpriteManager.h"
#include "SpriteResource.h"
#include <cassert>
#include <d3d12.h>
#include <wrl/client.h>

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }
} // namespace

// DirectX依存のメンバを隠蔽する構造体
struct Sprite::Impl {
  Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> transformBuffer;
  MaterialCB *materialMapped = nullptr;
  TransformCB *transformMapped = nullptr;
  std::shared_ptr<SpriteResource> resource;
};

Sprite::Sprite() : pImpl_(std::make_unique<Impl>()) {}
Sprite::~Sprite() = default;

bool Sprite::Initialize(const CreateInfo &info) {
  auto *renderer = Renderer::GetInstance();

  // 1. SpriteManager経由でリソースを取得
  pImpl_->resource = SpriteManager::GetInstance()->Load(info.texturePath);
  if (!pImpl_->resource)
    return false;

  // 2. 定数バッファ作成 (Rendererに依頼)
  pImpl_->materialBuffer =
      renderer->CreateUploadBuffer(Align256_(sizeof(MaterialCB)));
  pImpl_->materialBuffer->Map(
      0, nullptr, reinterpret_cast<void **>(&pImpl_->materialMapped));

  pImpl_->transformBuffer =
      renderer->CreateUploadBuffer(Align256_(sizeof(TransformCB)));
  pImpl_->transformBuffer->Map(
      0, nullptr, reinterpret_cast<void **>(&pImpl_->transformMapped));

  // 初期値
  color_ = info.color;
  scale_ = {info.size.x, info.size.y, 1.0f};

  return true;
}

void Sprite::Draw() {
  if (!pImpl_->materialMapped || !pImpl_->transformMapped)
    return;

  // 行列の更新
  worldMatrix_ = MakeAffineMatrix(scale_, rotation_, position_);

  // 定数バッファの書き換え (Worldのみ。WVPはRendererで合成)
  pImpl_->materialMapped->color = color_;
  pImpl_->materialMapped->uvTransform = uvMatrix_;
  pImpl_->transformMapped->World = worldMatrix_;

  // Rendererに自分自身の描画を依頼
  Renderer::GetInstance()->DrawSprite(this);
}

unsigned long long Sprite::GetMaterialCBAddress() const {
  return pImpl_->materialBuffer->GetGPUVirtualAddress();
}

unsigned long long Sprite::GetTransformCBAddress() const {
  return pImpl_->transformBuffer->GetGPUVirtualAddress();
}

SpriteResource *Sprite::GetResource() const { return pImpl_->resource.get(); }

Sprite::TransformCB *Sprite::GetTransformMapped() {
  return pImpl_->transformMapped;
}