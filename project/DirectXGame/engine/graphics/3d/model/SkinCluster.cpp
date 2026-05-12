#include "SkinCluster.h"
#include "ModelUtils.h"
#include "Method.h"
#include "DirectXCommon.h"
#include "Renderer.h"

bool SkinCluster::Initialize(DirectXCommon* dx, uint32_t jointCount) {
  auto* device = dx->GetDevice();
  auto& srvAlloc = dx->GetSrvAllocator();

  size_t size = sizeof(WellForGPU) * jointCount;
  paletteResource = Renderer::GetInstance()->CreateBuffer(size);
  if (!paletteResource) return false;

  HRESULT hr = paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
  if (FAILED(hr)) return false;

  // Initialize with identity
  for (uint32_t i = 0; i < jointCount; ++i) {
    mappedPalette[i].skeletonSpaceMatrix = MakeIdentity4x4();
    mappedPalette[i].skeletonSpaceInverseTransposeMatrix = MakeIdentity4x4();
  }

  // Create SRV
  srvIndex = srvAlloc.Allocate();
  
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = DXGI_FORMAT_UNKNOWN;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srvDesc.Buffer.FirstElement = 0;
  srvDesc.Buffer.NumElements = jointCount;
  srvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
  srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

  device->CreateShaderResourceView(paletteResource.Get(), &srvDesc, srvAlloc.Cpu(srvIndex));

  return true;
}

void SkinCluster::Update(const Skeleton& skeleton) {
  if (!mappedPalette) return;

  // Update palette
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    const Joint& joint = skeleton.joints[i];
    Matrix4x4 finalMatrix = Multiply(joint.inverseBindPoseMatrix, joint.skeletonSpaceMatrix);
    mappedPalette[i].skeletonSpaceMatrix = finalMatrix;
    mappedPalette[i].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(finalMatrix));
  }
}
