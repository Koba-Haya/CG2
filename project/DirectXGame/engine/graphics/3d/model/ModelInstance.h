#pragma once
#include "BlendMode.h"
#include "Method.h"
#include <memory>
#include <string>

// 前方宣言
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

  // HLSL 定数バッファ構造体 (Renderer が書き込むために公開)
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

  ModelInstance();
  ~ModelInstance(); // 重要：実装は .cpp へ

  bool Initialize(const CreateInfo &ci);

  // トランスフォーム設定
  void SetWorld(const Matrix4x4 &world);
  const Matrix4x4 &GetWorld() const { return world_; }

  // マテリアル設定
  void SetColor(const Vector4 &c);
  void SetLightingMode(int32_t m);
  void SetUVTransform(const Matrix4x4 &uv);
  void SetSpecularColor(const Vector3 &c);
  void SetShininess(float s);

  void SetWireframe(bool wireframe) { isWireframe_ = wireframe; }
  bool IsWireframe() const { return isWireframe_; }

  void Draw();

  // Renderer 用アクセサ
  unsigned long long GetMaterialCBAddress() const;
  unsigned long long GetTransformCBAddress() const;
  ModelResource *GetResource() const;

  // Renderer が WVP を最終合成・転送するためのポインタ取得
  TransformCB *GetTransformMapped();

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl_;

  Matrix4x4 world_ = MakeIdentity4x4();
  bool isWireframe_ = false;
};