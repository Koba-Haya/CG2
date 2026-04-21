#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <memory>
#include <string>
#include <vector>

#include "DirectXCommon.h"
#include "Matrix.h"
#include "TextureResource.h"
#include "UnifiedPipeline.h"
#include "Vector.h"
#include "Method.h"

class Skybox {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  struct Vertex {
    Vector4 position;
  };

  struct MaterialCB {
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
  };

  struct TransformCB {
    Matrix4x4 WVP = MakeIdentity4x4();
    Matrix4x4 World = MakeIdentity4x4();
    Matrix4x4 WorldInverseTranspose = MakeIdentity4x4();
  };

public:
  Skybox() = default;
  ~Skybox() = default;

  bool Initialize(DirectXCommon *dx, UnifiedPipeline *pipeline,
                  const std::string &texturePath);

  void Update(const Matrix4x4 &viewMatrix, const Matrix4x4 &projectionMatrix,
              const Vector3 &cameraPosition, const Vector3 &scale);

  void Draw(ID3D12GraphicsCommandList *cmdList);

  void SetColor(const Vector4 &color) {
    if (materialMapped_) {
      materialMapped_->color = color;
    }
  }

  std::shared_ptr<TextureResource> GetTexture() const { return texture_; }

private:
  static void CreateVertices_(std::vector<Vertex> &vertices,
                              std::vector<uint32_t> &indices);

  static ComPtr<ID3D12Resource> CreateUploadBuffer_(ID3D12Device *device,
                                                    size_t size);

private:
  DirectXCommon *dx_ = nullptr;
  UnifiedPipeline *pipeline_ = nullptr;

  ComPtr<ID3D12Resource> vb_;
  D3D12_VERTEX_BUFFER_VIEW vbView_{};

  ComPtr<ID3D12Resource> ib_;
  D3D12_INDEX_BUFFER_VIEW ibView_{};
  UINT indexCount_ = 0;

  ComPtr<ID3D12Resource> materialCB_;
  MaterialCB *materialMapped_ = nullptr;

  ComPtr<ID3D12Resource> transformCB_;
  TransformCB *transformMapped_ = nullptr;

  std::shared_ptr<TextureResource> texture_;
};