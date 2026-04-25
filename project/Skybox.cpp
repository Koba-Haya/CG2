#include "Skybox.h"
#include "Renderer.h"

#include <cassert>
#include <cstring>

namespace {
static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }
} // namespace

Microsoft::WRL::ComPtr<ID3D12Resource>
Skybox::CreateUploadBuffer_(ID3D12Device *device, size_t size) {
  ComPtr<ID3D12Resource> resource;

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = static_cast<UINT64>(size);
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  HRESULT hr = device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));

  return resource;
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

bool Skybox::Initialize(const std::string &texturePath) {
  dx_ = Renderer::GetInstance()->GetDX();
  if (!dx_) {
    return false;
  }

  ID3D12Device *device = dx_->GetDevice();
  if (!device) {
    return false;
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  CreateVertices_(vertices, indices);
  indexCount_ = static_cast<UINT>(indices.size());

  const size_t vbSize = sizeof(Vertex) * vertices.size();
  vb_ = CreateUploadBuffer_(device, vbSize);
  if (!vb_) {
    return false;
  }

  {
    void *mapped = nullptr;
    HRESULT hr = vb_->Map(0, nullptr, &mapped);
    if (FAILED(hr)) {
      return false;
    }
    std::memcpy(mapped, vertices.data(), vbSize);
    vb_->Unmap(0, nullptr);
  }

  vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
  vbView_.StrideInBytes = sizeof(Vertex);
  vbView_.SizeInBytes = static_cast<UINT>(vbSize);

  const size_t ibSize = sizeof(uint32_t) * indices.size();
  ib_ = CreateUploadBuffer_(device, ibSize);
  if (!ib_) {
    return false;
  }

  {
    void *mapped = nullptr;
    HRESULT hr = ib_->Map(0, nullptr, &mapped);
    if (FAILED(hr)) {
      return false;
    }
    std::memcpy(mapped, indices.data(), ibSize);
    ib_->Unmap(0, nullptr);
  }

  ibView_.BufferLocation = ib_->GetGPUVirtualAddress();
  ibView_.SizeInBytes = static_cast<UINT>(ibSize);
  ibView_.Format = DXGI_FORMAT_R32_UINT;

  materialCB_ = CreateUploadBuffer_(device, Align256_(sizeof(MaterialCB)));
  if (!materialCB_) {
    return false;
  }

  {
    HRESULT hr = materialCB_->Map(0, nullptr,
                                  reinterpret_cast<void **>(&materialMapped_));
    if (FAILED(hr)) {
      return false;
    }
    materialMapped_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  }

  transformCB_ = CreateUploadBuffer_(device, Align256_(sizeof(TransformCB)));
  if (!transformCB_) {
    return false;
  }

  {
    HRESULT hr = transformCB_->Map(
        0, nullptr, reinterpret_cast<void **>(&transformMapped_));
    if (FAILED(hr)) {
      return false;
    }
    *transformMapped_ = {};
  }

  texture_ = std::make_shared<TextureResource>();
  if (!texture_->CreateFromFile(dx_, texturePath)) {
    texture_.reset();
    return false;
  }

  return true;
}

void Skybox::Update(const Matrix4x4 &viewMatrix,
                    const Matrix4x4 &projectionMatrix,
                    const Vector3 &cameraPosition, const Vector3 &scale) {
  if (!transformMapped_) {
    return;
  }

  Matrix4x4 worldMatrix =
      MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, cameraPosition);

  transformMapped_->World = worldMatrix;
  transformMapped_->WVP =
      Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
  transformMapped_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
}

void Skybox::Draw() {
  Renderer::GetInstance()->DrawSkybox(this);
}

void Skybox::DrawInternal(ID3D12GraphicsCommandList *cmdList) {
  if (!cmdList || !dx_ || !vb_ || !ib_ || !materialCB_ ||
      !transformCB_ || !texture_) {
    return;
  }

  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdList->IASetVertexBuffers(0, 1, &vbView_);
  cmdList->IASetIndexBuffer(&ibView_);

  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);

  cmdList->SetGraphicsRootConstantBufferView(
      0, materialCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(
      1, transformCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootDescriptorTable(2, texture_->GetSrvGpu());

  cmdList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}