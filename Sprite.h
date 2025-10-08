#pragma once

#include <Windows.h>
#include <cassert>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "DirectXCommon.h"
#include "Matrix.h" // Matrix4x4 用
#include "Method.h" // 行列計算関数（MakeAffineMatrixなど）
#include "UnifiedPipeline.h"
#include "Vector.h" // Vector2, Vector3, Vector4 用

class Sprite {
public:
  struct CreateInfo {
    DirectXCommon *dx = nullptr;
    UnifiedPipeline *pipeline = nullptr;
    SrvAllocator *srvAlloc = nullptr;
    std::string texturePath;
    Vector2 size = {640, 360};
    Vector4 color = {1, 1, 1, 1};
  };

  bool Initialize(const CreateInfo &info);
  void SetPosition(const Vector3 &pos);
  void SetScale(const Vector3 &scale);
  void SetRotation(const Vector3 &rot);
  void SetUVTransform(const Matrix4x4 &uv);
  void SetColor(const Vector4 &color);
  void Draw(const Matrix4x4 &view, const Matrix4x4 &proj);

private:
  // 省略しない ComPtr エイリアス
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
  ComPtr<ID3D12Resource> texture_;

  DirectXCommon *dx_ = nullptr;
  UnifiedPipeline *pipeline_ = nullptr;
  SrvAllocator *srvAlloc_ = nullptr;

  ComPtr<ID3D12Resource> vertexBuffer_;
  D3D12_VERTEX_BUFFER_VIEW vbView_{};
  ComPtr<ID3D12Resource> indexBuffer_;
  D3D12_INDEX_BUFFER_VIEW ibView_{};
  ComPtr<ID3D12Resource> materialBuffer_;
  ComPtr<ID3D12Resource> transformBuffer_;

  D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

  Matrix4x4 worldMatrix_ = MakeIdentity4x4();
  Vector3 position_{0, 0, 0};
  Vector3 scale_{1, 1, 1};
  Vector3 rotation_{0, 0, 0};
  Matrix4x4 uvMatrix_ = MakeIdentity4x4(); // UV用行列
  Vector4 color_ = {1, 1, 1, 1};           // マテリアルカラー
};
