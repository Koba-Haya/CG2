#include "SkinCluster.h"
#include "ModelUtils.h"
#include "Method.h"
#include "DirectXCommon.h"
#include "Renderer.h"

bool SkinCluster::Initialize(DirectXCommon* dx) {
  auto* renderer = Renderer::GetInstance();
  if (!renderer) return false;

  size_t size = sizeof(Matrix4x4) * kMaxBones;
  paletteResource = renderer->CreateBuffer(size);
  if (!paletteResource) return false;

  HRESULT hr = paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
  if (FAILED(hr)) return false;

  // Initialize with identity
  for (int i = 0; i < kMaxBones; ++i) {
    mappedPalette[i] = MakeIdentity4x4();
  }

  return true;
}

void SkinCluster::Update(const Skeleton& skeleton) {
  if (!mappedPalette) return;

  // Update palette
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    if (i >= kMaxBones) break;
    const Joint& joint = skeleton.joints[i];
    Matrix4x4 finalMatrix = Multiply(joint.inverseBindPoseMatrix, joint.skeletonSpaceMatrix);
    mappedPalette[i] = finalMatrix;
  }
}
