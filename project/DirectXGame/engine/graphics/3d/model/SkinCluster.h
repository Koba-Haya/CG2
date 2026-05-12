#pragma once

#include "Matrix.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>

class DirectXCommon;
struct Skeleton;

struct WellForGPU {
  Matrix4x4 skeletonSpaceMatrix;
  Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinCluster {
  Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
  WellForGPU* mappedPalette = nullptr;
  uint32_t srvIndex = 0;
  
  bool Initialize(DirectXCommon* dx, uint32_t jointCount);
  void Update(const Skeleton& skeleton);
};
