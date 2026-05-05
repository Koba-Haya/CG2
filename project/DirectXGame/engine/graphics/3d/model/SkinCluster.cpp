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

  std::vector<Matrix4x4> globalMatrices(skeleton.joints.size());
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    globalMatrices[i] = MakeIdentity4x4();
  }

  // Calculate global matrices
  // Assumes joints are topologically sorted, but just to be safe, recursive calculation is better if not sorted.
  // We'll use a recursive lambda.
  auto calculateGlobalMatrix = [&](auto& self, int32_t jointIndex, const Matrix4x4& parentMatrix) -> void {
    if (jointIndex < 0 || jointIndex >= static_cast<int32_t>(skeleton.joints.size())) return;

    const Joint& joint = skeleton.joints[jointIndex];
    Matrix4x4 currentGlobal = Multiply(joint.localMatrix, parentMatrix);
    globalMatrices[jointIndex] = currentGlobal;

    for (int32_t childIndex : joint.childrenIndices) {
      self(self, childIndex, currentGlobal);
    }
  };

  if (skeleton.rootJointIndex != -1) {
    calculateGlobalMatrix(calculateGlobalMatrix, skeleton.rootJointIndex, MakeIdentity4x4());
  }

  // Update palette
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    if (i >= kMaxBones) break;
    const Joint& joint = skeleton.joints[i];
    Matrix4x4 finalMatrix = Multiply(joint.inverseBindPoseMatrix, globalMatrices[i]);
    mappedPalette[i] = finalMatrix;
  }
}
