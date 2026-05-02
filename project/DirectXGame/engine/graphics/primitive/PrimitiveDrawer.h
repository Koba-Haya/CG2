#pragma once
#include "Vector.h"
#include <memory>
#include <vector>

class DirectXCommon;
struct ID3D12GraphicsCommandList;

struct PrimitiveVertex {
  Vector3 pos;
  Vector4 color;
};

class PrimitiveDrawer {
public:
  PrimitiveDrawer();
  ~PrimitiveDrawer();

  void Initialize(DirectXCommon *dx, uint32_t maxVertices = 1000000);

  // 描画予約メソッド
  void AddLine(const Vector3 &start, const Vector3 &end, const Vector4 &color);
  void AddGrid(float size, int divisions, const Vector4 &color);

  // 内部処理
  void Reset(); // 毎フレームの最初に呼ぶ
  void Draw(ID3D12GraphicsCommandList *cmdList);

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl_;
};