#pragma once
#include "DirectXCommon.h"   // デバイス/コマンド/ヒープ類
#include "UnifiedPipeline.h" // RootSig / PSO
#include "Vector.h" // Vector2, Vector3, Vector4 用
#include "Matrix.h" // Matrix4x4 用
#include "Method.h" // 行列計算関数（MakeAffineMatrixなど）
#include <assert.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
  Vector3 normal;
};

struct Material {
  Vector4 color;          // RGBA
  int32_t enableLighting; // 0=Unlit,1=Lambert,2=HalfLambert
  float pad[3];
  Matrix4x4 uvTransform;
};

struct Transformation {
  Matrix4x4 WVP;
  Matrix4x4 World;
};

struct DirectionalLight {
  Vector4 color;
  Vector3 direction;
  float intensity;
};

// 既存の OBJ ローダが返すものを流用
struct MaterialData {
  std::string textureFilePath;
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
};

class Model {
public:
  // 作成に必要な情報をまとめる
  struct CreateInfo {
    DirectXCommon *dx = nullptr; // デバイス/コマンド/ヒープへのアクセス
    UnifiedPipeline *pipeline = nullptr; // 使用するPSO（Object3D用）
    SrvAllocator *srvAlloc = nullptr;    // SRV割当器（ImGuiと共用）
    ModelData modelData; // 頂点配列 & マテリアル（テクスチャパス等）
    Vector4 baseColor{1, 1, 1, 1}; // デフォルト色
    int32_t lightingMode = 1;      // 0/1/2（PSの分岐と一致）
  };

public:
  bool Initialize(const CreateInfo &ci);
  void SetColor(const Vector4 &color);
  void SetLightingMode(int32_t mode); // 0=Unlit, 1=Lambert, 2=HalfLambert
  void SetUVTransform(const Matrix4x4 &uv);
  void SetWorldTransform(const Matrix4x4 &world);

  // view/proj と ライトCB の GPU アドレスは呼び出し側から（共通）渡す
  void Draw(const Matrix4x4 &view, const Matrix4x4 &proj,
            ID3D12Resource *directionalLightCB);

private:
  // 省略しない ComPtr エイリアス
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  // ヘルパ
  ComPtr<ID3D12Resource> CreateUploadBuffer(size_t size);
  void CreateTextureFromFile(const std::string &path);

private:
  // 参照
  DirectXCommon *dx_ = nullptr;
  UnifiedPipeline *pipeline_ = nullptr;
  SrvAllocator *srvAlloc_ = nullptr;

  // リソース
  ComPtr<ID3D12Resource> vb_;
  D3D12_VERTEX_BUFFER_VIEW vbv_{};
  ComPtr<ID3D12Resource> cbMaterial_;
  Material *cbMatMapped_ = nullptr;
  ComPtr<ID3D12Resource> cbTransform_;
  Transformation *cbTransMapped_ = nullptr;
  ComPtr<ID3D12Resource> tex_;
  UINT texSrvIndex_ = 0;
  D3D12_GPU_DESCRIPTOR_HANDLE texSrvHandleGPU_{};

  // 行列
  Matrix4x4 world_ = MakeIdentity4x4();
};
