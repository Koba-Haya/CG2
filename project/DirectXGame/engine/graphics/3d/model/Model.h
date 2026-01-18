#pragma once
#include <d3d12.h>
#include <memory>
#include <wrl.h>

#include "DirectXCommon.h"
#include "Matrix.h"
#include "Method.h"
#include "UnifiedPipeline.h"
#include "Vector.h"

#include "ModelUtils.h"
#include "TextureResource.h"

// ===== CB layouts (HLSLと揃える) =====
struct ModelMaterialCB {
  Vector4 color{1, 1, 1, 1};
  int32_t enableLighting{1};
  Vector3 specularColor{1.0f, 1.0f, 1.0f};
  Matrix4x4 uvTransform = MakeIdentity4x4();
  float shininess{32.0f};
  float pad[3]{0, 0, 0};
};

struct ModelTransformCB {
  Matrix4x4 WVP = MakeIdentity4x4();
  Matrix4x4 World = MakeIdentity4x4();
  Matrix4x4 WorldInverseTranspose = MakeIdentity4x4();
};

class Model {
public:
  struct CreateInfo {
    DirectXCommon *dx = nullptr;
    UnifiedPipeline *pipeline = nullptr;
    ModelData modelData;
    Vector4 baseColor = {1, 1, 1, 1};
    int32_t lightingMode = 1;
  };

  bool Initialize(const CreateInfo &ci);

  void SetWorldTransform(const Matrix4x4 &world);
  void SetColor(const Vector4 &color);

  void SetLightingMode(int32_t mode);
  void SetUVTransform(const Matrix4x4 &uv);

  void SetSpecularColor(const Vector3 &color);
  void SetShininess(float shininess);

  void Draw(const Matrix4x4 &view, const Matrix4x4 &proj,
            ID3D12Resource *directionalLightCB, ID3D12Resource *cameraCB);

private:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
  ComPtr<ID3D12Resource> CreateUploadBuffer(size_t size);

private:
  DirectXCommon *dx_ = nullptr;
  UnifiedPipeline *pipeline_ = nullptr;

  std::shared_ptr<TextureResource> texture_;
  D3D12_GPU_DESCRIPTOR_HANDLE texSrvHandleGPU_{};

  ComPtr<ID3D12Resource> vb_;
  D3D12_VERTEX_BUFFER_VIEW vbv_{};

  ComPtr<ID3D12Resource> cbMaterial_;
  ModelMaterialCB *cbMatMapped_ = nullptr;

  ComPtr<ID3D12Resource> cbTransform_;
  ModelTransformCB *cbTransMapped_ = nullptr;

  Matrix4x4 world_ = MakeIdentity4x4();
  uint32_t vertexCount_ = 0;
};
