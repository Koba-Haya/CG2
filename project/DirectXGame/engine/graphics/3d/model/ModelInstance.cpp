#include "ModelInstance.h"
#include "ModelResource.h"
#include "Renderer.h"
#include <cassert>
#include <d3d12.h>
#include <wrl/client.h>
#include "SkinCluster.h"
#include "Animation.h"
#include "ModelUtils.h"

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }
} // namespace

struct ModelInstance::Impl {
  Microsoft::WRL::ComPtr<ID3D12Resource> cbMaterial;
  Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform;
  MaterialCB *cbMatMapped = nullptr;
  TransformCB *cbTransMapped = nullptr;
  std::shared_ptr<ModelResource> resource;
  
  std::shared_ptr<Animation> currentAnimation;
  float animationTime = 0.0f;
  bool animationLoop = true;
  std::unique_ptr<Skeleton> skeleton;
  std::unique_ptr<SkinCluster> skinCluster;
};

ModelInstance::ModelInstance() : pImpl_(std::make_unique<Impl>()) {}
ModelInstance::~ModelInstance() = default;

ModelInstance::ModelInstance(ModelInstance &&) noexcept = default;
ModelInstance &ModelInstance::operator=(ModelInstance &&) noexcept = default;

bool ModelInstance::Initialize(const CreateInfo &ci) {
  auto *renderer = Renderer::GetInstance();
  assert(renderer && ci.resource);
  pImpl_->resource = ci.resource;

  world_ = MakeIdentity4x4();

  pImpl_->cbMaterial =
      renderer->CreateUploadBuffer(Align256_(sizeof(MaterialCB)));
  pImpl_->cbMaterial->Map(0, nullptr,
                          reinterpret_cast<void **>(&pImpl_->cbMatMapped));

  pImpl_->cbTransform =
      renderer->CreateUploadBuffer(Align256_(sizeof(TransformCB)));
  pImpl_->cbTransform->Map(0, nullptr,
                           reinterpret_cast<void **>(&pImpl_->cbTransMapped));

  *pImpl_->cbMatMapped = {};
  pImpl_->cbMatMapped->color = ci.baseColor;
  pImpl_->cbMatMapped->enableLighting = ci.lightingMode;
  pImpl_->cbMatMapped->specularColor = ci.specularColor;
  pImpl_->cbMatMapped->uvTransform = MakeIdentity4x4();
  pImpl_->cbMatMapped->shininess = ci.shininess;
  pImpl_->cbMatMapped->environmentCoefficient =
      ci.environmentCoefficient; // 追加

  SetWorld(MakeIdentity4x4());

  if (pImpl_->resource->HasBones()) {
    pImpl_->skeleton = std::make_unique<Skeleton>(pImpl_->resource->GetModelData()->skeleton);
    pImpl_->skinCluster = std::make_unique<SkinCluster>();
    pImpl_->skinCluster->Initialize(renderer->GetDX());
  }

  return true;
}

void ModelInstance::SetWorld(const Matrix4x4 &world) {
  world_ = world;
  if (pImpl_->cbTransMapped) {
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
void ModelInstance::SetEnvironmentCoefficient(float c) { // 追加
  if (pImpl_->cbMatMapped)
    pImpl_->cbMatMapped->environmentCoefficient = c;
}

void ModelInstance::Draw() { Renderer::GetInstance()->DrawModel(this); }

void ModelInstance::DrawSkeleton() {
  if (!pImpl_->skeleton) return;
  auto* renderer = Renderer::GetInstance();
  for (const auto& joint : pImpl_->skeleton->joints) {
    // 自分自身の位置 (平行移動成分を抽出)
    Vector3 start = { joint.skeletonSpaceMatrix.m[3][0], joint.skeletonSpaceMatrix.m[3][1], joint.skeletonSpaceMatrix.m[3][2] };
    start = TransformPoint(start, world_);
    
    // 子への線を描く
    for (int32_t childIndex : joint.children) {
      const auto& child = pImpl_->skeleton->joints[childIndex];
      Vector3 end = { child.skeletonSpaceMatrix.m[3][0], child.skeletonSpaceMatrix.m[3][1], child.skeletonSpaceMatrix.m[3][2] };
      end = TransformPoint(end, world_);
      
      renderer->DrawLine(start, end, {1.0f, 1.0f, 1.0f, 1.0f});
    }
  }
}

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

SkinCluster* ModelInstance::GetSkinCluster() const {
  return pImpl_->skinCluster.get();
}

Skeleton* ModelInstance::GetSkeleton() const {
  return pImpl_->skeleton.get();
}

template<typename T>
T CalculateValue(const std::vector<Keyframe<T>>& keyframes, float time) {
  assert(!keyframes.empty());
  if (keyframes.size() == 1 || time <= keyframes[0].time) {
    return keyframes[0].value;
  }
  for (size_t index = 0; index < keyframes.size() - 1; ++index) {
    size_t nextIndex = index + 1;
    if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
      float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
      if constexpr (std::is_same_v<T, Vector3>) {
        return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
      } else if constexpr (std::is_same_v<T, Quaternion>) {
        return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
      }
    }
  }
  return (*keyframes.rbegin()).value;
}

void ModelInstance::PlayAnimation(std::shared_ptr<Animation> animation, bool loop) {
  pImpl_->currentAnimation = animation;
  pImpl_->animationTime = 0.0f;
  pImpl_->animationLoop = loop;
}

void ModelInstance::UpdateAnimation(float deltaTime) {
  if (!pImpl_->currentAnimation || !pImpl_->skeleton) return;
  
  pImpl_->animationTime += deltaTime;
  if (pImpl_->animationLoop) {
    pImpl_->animationTime = std::fmod(pImpl_->animationTime, pImpl_->currentAnimation->duration);
  } else {
    pImpl_->animationTime = std::min(pImpl_->animationTime, pImpl_->currentAnimation->duration);
  }

  for (auto& joint : pImpl_->skeleton->joints) {
    if (pImpl_->currentAnimation->nodeAnimations.find(joint.name) != pImpl_->currentAnimation->nodeAnimations.end()) {
      const auto& nodeAnim = pImpl_->currentAnimation->nodeAnimations[joint.name];
      
      if (!nodeAnim.translate.keyframes.empty()) joint.transform.translate = CalculateValue(nodeAnim.translate.keyframes, pImpl_->animationTime);
      if (!nodeAnim.rotate.keyframes.empty()) joint.transform.rotate = CalculateValue(nodeAnim.rotate.keyframes, pImpl_->animationTime);
      if (!nodeAnim.scale.keyframes.empty()) joint.transform.scale = CalculateValue(nodeAnim.scale.keyframes, pImpl_->animationTime);
    }
  }

  UpdateSkeleton(*pImpl_->skeleton);

  if (pImpl_->skinCluster) {
    pImpl_->skinCluster->Update(*pImpl_->skeleton);
  }
}