#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <memory>

#include "Matrix.h"
#include "Method.h"
#include "Vector.h"
#include "UnifiedPipeline.h"

class DirectXCommon;
class ModelResource;

class ModelInstance {
public:
  struct CreateInfo {
    std::shared_ptr<ModelResource> resource;

    Vector4 baseColor = {1, 1, 1, 1};
    int32_t lightingMode = 1;

    Vector3 specularColor = {1.0f, 1.0f, 1.0f};
    float shininess = 32.0f;
  };

  bool Initialize(const CreateInfo &ci);

  void SetWorld(const Matrix4x4 &world) { world_ = world; }

  void SetColor(const Vector4 &c);
  void SetLightingMode(int32_t m);
  void SetUVTransform(const Matrix4x4 &uv);

  void SetSpecularColor(const Vector3 &c);
  void SetShininess(float s);

  void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
  BlendMode GetBlendMode() const { return blendMode_; }

  void SetWireframe(bool wireframe) { isWireframe_ = wireframe; }
  bool IsWireframe() const { return isWireframe_; }

  void Draw();

  // Called by Renderer
  void DrawInternal(ID3D12GraphicsCommandList* cmdList, const Matrix4x4 &view, const Matrix4x4 &proj);

private:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
  ComPtr<ID3D12Resource> CreateUploadBuffer_(size_t size);

  struct MaterialCB {
    Vector4 color{1, 1, 1, 1};
    int32_t enableLighting = 1;

    Vector3 specularColor{1.0f, 1.0f, 1.0f};

    Matrix4x4 uvTransform = MakeIdentity4x4();

    float shininess = 32.0f;
    float pad[3] = {0, 0, 0};
  };

  struct TransformCB {
    Matrix4x4 WVP = MakeIdentity4x4();
    Matrix4x4 World = MakeIdentity4x4();
    Matrix4x4 WorldInverseTranspose = MakeIdentity4x4();
  };

private:
  DirectXCommon *dx_ = nullptr;
  UnifiedPipeline *pipeline_ = nullptr;
  std::shared_ptr<ModelResource> resource_;

  Matrix4x4 world_ = MakeIdentity4x4();

  ComPtr<ID3D12Resource> cbMaterial_;
  MaterialCB *cbMatMapped_ = nullptr;

  ComPtr<ID3D12Resource> cbTransform_;
  TransformCB *cbTransMapped_ = nullptr;

  BlendMode blendMode_ = BlendMode::Opaque;
  bool isWireframe_ = false;
};
