#include "Skybox.h"
#include "Renderer.h"
#include "TextureManager.h"
#include "TextureResource.h"
#include <d3d12.h>
#include <cassert>

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }
} // namespace

bool Skybox::Initialize(const std::string &texturePath) {
  auto *renderer = Renderer::GetInstance();

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  CreateVertices_(vertices, indices);
  indexCount_ = static_cast<UINT>(indices.size());

  // 1. 頂点バッファ
  const size_t vbSize = sizeof(Vertex) * vertices.size();
  vb_ = renderer->CreateUploadBuffer(vbSize);
  void *vbData = nullptr;
  vb_->Map(0, nullptr, &vbData);
  std::memcpy(vbData, vertices.data(), vbSize);
  vb_->Unmap(0, nullptr);

  // 2. インデックスバッファ
  const size_t ibSize = sizeof(uint32_t) * indices.size();
  ib_ = renderer->CreateUploadBuffer(ibSize);
  void *ibData = nullptr;
  ib_->Map(0, nullptr, &ibData);
  std::memcpy(ibData, indices.data(), ibSize);
  ib_->Unmap(0, nullptr);

  // 3. 定数バッファ
  materialCB_ = renderer->CreateUploadBuffer(Align256_(sizeof(MaterialCB)));
  materialCB_->Map(0, nullptr, reinterpret_cast<void **>(&materialMapped_));
  *materialMapped_ = MaterialCB{};

  transformCB_ = renderer->CreateUploadBuffer(Align256_(sizeof(TransformCB)));
  transformCB_->Map(0, nullptr, reinterpret_cast<void **>(&transformMapped_));
  *transformMapped_ = TransformCB{};

  // 4. テクスチャ
  texture_ = TextureManager::GetInstance()->Load(texturePath);

  return true;
}

void Skybox::Update(const Matrix4x4 &view, const Matrix4x4 &proj,
                    const Vector3 &camPos, const Vector3 &scale) {
  if (!transformMapped_)
    return;
  Matrix4x4 world = MakeAffineMatrix(scale, {0, 0, 0}, camPos);
  transformMapped_->World = world;
  transformMapped_->WVP = Multiply(world, Multiply(view, proj));
  transformMapped_->WorldInverseTranspose = Transpose(Inverse(world));
}

void Skybox::Draw() { Renderer::GetInstance()->DrawSkybox(this); }

// アクセサ群
unsigned long long Skybox::GetVBAddress() const {
  return vb_->GetGPUVirtualAddress();
}
unsigned int Skybox::GetVBSize() const {
  return (unsigned int)vb_->GetDesc().Width;
}
unsigned int Skybox::GetVBStride() const { return sizeof(Vertex); }
unsigned long long Skybox::GetIBAddress() const {
  return ib_->GetGPUVirtualAddress();
}
unsigned int Skybox::GetIBSize() const {
  return (unsigned int)ib_->GetDesc().Width;
}
unsigned int Skybox::GetIndexCount() const { return indexCount_; }
unsigned long long Skybox::GetMaterialCBAddress() const {
  return materialCB_->GetGPUVirtualAddress();
}
unsigned long long Skybox::GetTransformCBAddress() const {
  return transformCB_->GetGPUVirtualAddress();
}

void Skybox::SetColor(const Vector4 &color) {
  if (materialMapped_)
    materialMapped_->color = color;
}

void Skybox::CreateVertices_(std::vector<Vertex> &vertices,
                             std::vector<uint32_t> &indices) {
  vertices.resize(24);

  indices = {
      // right
      0,
      1,
      2,
      2,
      1,
      3,
      // left
      4,
      5,
      6,
      6,
      5,
      7,
      // front
      8,
      9,
      10,
      10,
      9,
      11,
      // back
      12,
      13,
      14,
      14,
      13,
      15,
      // top
      16,
      17,
      18,
      18,
      17,
      19,
      // bottom
      20,
      21,
      22,
      22,
      21,
      23,
  };

  // right (+X)
  vertices[0].position = {1.0f, 1.0f, 1.0f, 1.0f};
  vertices[1].position = {1.0f, 1.0f, -1.0f, 1.0f};
  vertices[2].position = {1.0f, -1.0f, 1.0f, 1.0f};
  vertices[3].position = {1.0f, -1.0f, -1.0f, 1.0f};

  // left (-X)
  vertices[4].position = {-1.0f, 1.0f, -1.0f, 1.0f};
  vertices[5].position = {-1.0f, 1.0f, 1.0f, 1.0f};
  vertices[6].position = {-1.0f, -1.0f, -1.0f, 1.0f};
  vertices[7].position = {-1.0f, -1.0f, 1.0f, 1.0f};

  // front (+Z)
  vertices[8].position = {-1.0f, 1.0f, 1.0f, 1.0f};
  vertices[9].position = {1.0f, 1.0f, 1.0f, 1.0f};
  vertices[10].position = {-1.0f, -1.0f, 1.0f, 1.0f};
  vertices[11].position = {1.0f, -1.0f, 1.0f, 1.0f};

  // back (-Z)
  vertices[12].position = {1.0f, 1.0f, -1.0f, 1.0f};
  vertices[13].position = {-1.0f, 1.0f, -1.0f, 1.0f};
  vertices[14].position = {1.0f, -1.0f, -1.0f, 1.0f};
  vertices[15].position = {-1.0f, -1.0f, -1.0f, 1.0f};

  // top (+Y)
  vertices[16].position = {-1.0f, 1.0f, -1.0f, 1.0f};
  vertices[17].position = {1.0f, 1.0f, -1.0f, 1.0f};
  vertices[18].position = {-1.0f, 1.0f, 1.0f, 1.0f};
  vertices[19].position = {1.0f, 1.0f, 1.0f, 1.0f};

  // bottom (-Y)
  vertices[20].position = {-1.0f, -1.0f, 1.0f, 1.0f};
  vertices[21].position = {1.0f, -1.0f, 1.0f, 1.0f};
  vertices[22].position = {-1.0f, -1.0f, -1.0f, 1.0f};
  vertices[23].position = {1.0f, -1.0f, -1.0f, 1.0f};
}
