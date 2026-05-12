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
  
  // CS出力用
  Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexBuffer;
  uint32_t uavIndex = 0;
  D3D12_VERTEX_BUFFER_VIEW vbView{};

  bool Initialize(DirectXCommon* dx, uint32_t jointCount, uint32_t vertexCount);
  void Update(const Skeleton& skeleton);
};
