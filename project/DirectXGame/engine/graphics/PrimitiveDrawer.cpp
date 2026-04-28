#include "PrimitiveDrawer.h"
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>

// using Microsoft::WRL::ComPtr; // ← これを削除して、衝突を回避します

struct PrimitiveDrawer::Impl {
  DirectXCommon *dx = nullptr;
  // 直接名前空間を指定して記述
  Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;
  D3D12_VERTEX_BUFFER_VIEW vbView{};
  PrimitiveVertex *mappedVertices = nullptr;

  std::vector<PrimitiveVertex> vertices;
  uint32_t maxVertices = 0;
};

PrimitiveDrawer::PrimitiveDrawer() : pImpl_(std::make_unique<Impl>()) {}

PrimitiveDrawer::~PrimitiveDrawer() {
  if (pImpl_->vertBuff) {
    pImpl_->vertBuff->Unmap(0, nullptr);
  }
}

void PrimitiveDrawer::Initialize(DirectXCommon *dx, uint32_t maxVertices) {
  pImpl_->dx = dx;
  pImpl_->maxVertices = maxVertices;
  pImpl_->vertices.reserve(maxVertices);

  size_t sizeInBytes = sizeof(PrimitiveVertex) * maxVertices;

  auto device = dx->GetDevice();
  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = sizeInBytes;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                  IID_PPV_ARGS(&pImpl_->vertBuff));

  pImpl_->vertBuff->Map(0, nullptr,
                        reinterpret_cast<void **>(&pImpl_->mappedVertices));

  pImpl_->vbView.BufferLocation = pImpl_->vertBuff->GetGPUVirtualAddress();
  pImpl_->vbView.SizeInBytes = static_cast<UINT>(sizeInBytes);
  pImpl_->vbView.StrideInBytes = sizeof(PrimitiveVertex);
}

void PrimitiveDrawer::AddLine(const Vector3 &start, const Vector3 &end,
                              const Vector4 &color) {
  if (pImpl_->vertices.size() + 2 > pImpl_->maxVertices)
    return;
  pImpl_->vertices.push_back({start, color});
  pImpl_->vertices.push_back({end, color});
}

void PrimitiveDrawer::AddGrid(float size, int divisions, const Vector4 &color) {
  float step = size / divisions;
  float halfSize = size * 0.5f;
  for (int i = 0; i <= divisions; ++i) {
    float pos = -halfSize + i * step;
    // X方向の線
    AddLine({pos, 0.0f, -halfSize}, {pos, 0.0f, halfSize}, color);
    // Z方向の線
    AddLine({-halfSize, 0.0f, pos}, {halfSize, 0.0f, pos}, color);
  }
}

void PrimitiveDrawer::Reset() { pImpl_->vertices.clear(); }

void PrimitiveDrawer::Draw(ID3D12GraphicsCommandList *cmdList) {
  if (pImpl_->vertices.empty())
    return;

  std::memcpy(pImpl_->mappedVertices, pImpl_->vertices.data(),
              sizeof(PrimitiveVertex) * pImpl_->vertices.size());

  cmdList->IASetVertexBuffers(0, 1, &pImpl_->vbView);
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  cmdList->DrawInstanced(static_cast<UINT>(pImpl_->vertices.size()), 1, 0, 0);
}