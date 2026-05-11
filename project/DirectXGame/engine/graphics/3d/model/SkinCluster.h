#pragma once

#include "Matrix.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>

class DirectXCommon;
struct Skeleton;

static const int kMaxBones = 128;

struct SkinCluster {
  Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
  Matrix4x4* mappedPalette = nullptr;
  
  bool Initialize(DirectXCommon* dx);
  void Update(const Skeleton& skeleton);
};
