#include "ModelInstance.h"
#include "ModelResource.h"
#include "Renderer.h"
#include <cassert>
#include <d3d12.h>
#include <wrl/client.h>

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }
} // namespace

// DirectX 依存のメンバを隠蔽する構造体
struct ModelInstance::Impl {
  Microsoft::WRL::ComPtr<ID3D12Resource> cbMaterial;
  Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform;
  MaterialCB *cbMatMapped = nullptr;
  TransformCB *cbTransMapped = nullptr;
  std::shared_ptr<ModelResource> resource;
};

ModelInstance::ModelInstance() : pImpl_(std::make_unique<Impl>()) {}
ModelInstance::~ModelInstance() = default;

bool ModelInstance::Initialize(const CreateInfo &ci) {
  auto *renderer = Renderer::GetInstance();
  assert(renderer && ci.resource);
  pImpl_->resource = ci.resource;

  world_ = MakeIdentity4x4();

  // 1. マテリアルバッファ作成 (Renderer に依頼)
  pImpl_->cbMaterial =
      renderer->CreateUploadBuffer(Align256_(sizeof(MaterialCB)));
  pImpl_->cbMaterial->Map(0, nullptr,
                          reinterpret_cast<void **>(&pImpl_->cbMatMapped));

  // 2. トランスフォームバッファ作成
  pImpl_->cbTransform =
      renderer->CreateUploadBuffer(Align256_(sizeof(TransformCB)));
  pImpl_->cbTransform->Map(0, nullptr,
                           reinterpret_cast<void **>(&pImpl_->cbTransMapped));

  // 3. 初期値の設定
  *pImpl_->cbMatMapped = {};
  pImpl_->cbMatMapped->color = ci.baseColor;
  pImpl_->cbMatMapped->enableLighting = ci.lightingMode;
  pImpl_->cbMatMapped->specularColor = ci.specularColor;
  pImpl_->cbMatMapped->uvTransform = MakeIdentity4x4();
  pImpl_->cbMatMapped->shininess = ci.shininess;

  // 行列の初期設定
  SetWorld(MakeIdentity4x4());

  return true;
}

void ModelInstance::SetWorld(const Matrix4x4 &world) {
  world_ = world;
  if (pImpl_->cbTransMapped) {
    // CPU 側の変数に基づき、GPU 側の World 行列を即座に更新
    pImpl_->cbTransMapped->World = world_;
    pImpl_->cbTransMapped->WorldInverseTranspose = Transpose(Inverse(world_));
  }
}

void ModelInstance::SetColor(const Vector4 &c) {
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->color = c;
}
void ModelInstance::SetLightingMode(int32_t m) {
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->enableLighting = m;
}
void ModelInstance::SetUVTransform(const Matrix4x4 &uv) {
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->uvTransform = uv;
}
void ModelInstance::SetSpecularColor(const Vector3 &c) {
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->specularColor = c;
}
void ModelInstance::SetShininess(float s) {
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->shininess = s;
}

void ModelInstance::Draw() { Renderer::GetInstance()->DrawModel(this); }

unsigned long long ModelInstance::GetMaterialCBAddress() const {
  return pImpl_->cbMaterial->GetGPUVirtualAddress();
}

unsigned long long ModelInstance::GetTransformCBAddress() const {
  return pImpl_->cbTransform->GetGPUVirtualAddress();
}

ModelResource *ModelInstance::GetResource() const {
  return pImpl_->resource.get();
}

ModelInstance::TransformCB *ModelInstance::GetTransformMapped() {
  return pImpl_->cbTransMapped;
}