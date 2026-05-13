#pragma once
#include "BlendMode.h"
#include "Matrix.h"
#include "Method.h"
#include "Vector.h"
#include <memory>
#include <string>

// 前方宣言
class SpriteResource;

class Sprite {
public:
  struct CreateInfo {
    std::string texturePath;
    Vector2 size = {640.0f, 360.0f};
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
  };

  // インスタンス固有の定数バッファ構造体
  struct MaterialCB {
    Vector4 color;
    Matrix4x4 uvTransform;
  };
  struct TransformCB {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };

  Sprite();
  ~Sprite(); // 実装は .cpp へ

  bool Initialize(const CreateInfo &info);

  // トランスフォーム設定
  void SetPosition(const Vector3 &pos) { position_ = pos; }
  void SetScale(const Vector3 &scale) { scale_ = scale; }
  void SetRotation(const Vector3 &rot) { rotation_ = rot; }
  void SetUVTransform(const Matrix4x4 &uv) { uvMatrix_ = uv; }
  void SetColor(const Vector4 &color) { color_ = color; }
  void SetBlendMode(BlendMode mode) { blendMode_ = mode; }

  void Draw();

  // Renderer用アクセサ
  unsigned long long GetMaterialCBAddress() const;
  unsigned long long GetTransformCBAddress() const;
  SpriteResource *GetResource() const;
  BlendMode GetBlendMode() const { return blendMode_; }
  const Matrix4x4 &GetWorldMatrix() const { return worldMatrix_; }

  // RendererがWVPを計算・転送するための内部アクセサ
  TransformCB *GetTransformMapped();

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl_;

  // 状態パラメータ
  Vector3 position_ = {0, 0, 0};
  Vector3 scale_ = {1, 1, 1};
  Vector3 rotation_ = {0, 0, 0};
  Vector4 color_ = {1, 1, 1, 1};
  Matrix4x4 uvMatrix_ = MakeIdentity4x4();
  Matrix4x4 worldMatrix_ = MakeIdentity4x4();
  BlendMode blendMode_ = BlendMode::Alpha;
};