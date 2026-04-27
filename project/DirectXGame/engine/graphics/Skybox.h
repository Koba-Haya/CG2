#pragma once
#include "Method.h"
#include <memory>
#include <string>
#include <vector>
#include <wrl/client.h> // ComPtrのために必要

// 前方宣言
struct ID3D12Resource;
class TextureResource;

class Skybox {
public:
  // 頂点構造体
  struct Vertex {
    Vector4 position;
  };

  // 定数バッファ用構造体（実装側で使用するため定義を戻す）
  struct MaterialCB {
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
  };
  struct TransformCB {
    Matrix4x4 WVP = MakeIdentity4x4();
    Matrix4x4 World = MakeIdentity4x4();
    Matrix4x4 WorldInverseTranspose = MakeIdentity4x4();
  };

  bool Initialize(const std::string &texturePath);
  // 引数をGameSceneでの呼び出しに合わせて修正（scaleを追加）
  void Update(const Matrix4x4 &view, const Matrix4x4 &proj,
              const Vector3 &camPos, const Vector3 &scale = {1, 1, 1});
  void Draw();

  // Rendererが描画に使うアクセサ
  unsigned long long GetVBAddress() const;
  unsigned int GetVBSize() const;
  unsigned int GetVBStride() const;
  unsigned long long GetIBAddress() const;
  unsigned int GetIBSize() const;
  unsigned int GetIndexCount() const;
  unsigned long long GetMaterialCBAddress() const;
  unsigned long long GetTransformCBAddress() const;
  std::shared_ptr<TextureResource> GetTexture() const { return texture_; }

  void SetColor(const Vector4 &color);

private:
  static void CreateVertices_(std::vector<Vertex> &vertices,
                              std::vector<uint32_t> &indices);

private:
  // エイリアス
  template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  ComPtr<ID3D12Resource> vb_;
  ComPtr<ID3D12Resource> ib_;
  ComPtr<ID3D12Resource> materialCB_;
  ComPtr<ID3D12Resource> transformCB_;
  std::shared_ptr<TextureResource> texture_;

  unsigned int indexCount_ = 0;
  MaterialCB *materialMapped_ = nullptr;
  TransformCB *transformMapped_ = nullptr;
};