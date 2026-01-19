#include "Model.h"

#include <cassert>
#include <cstring>

#include "TextureManager.h"

template <class T> using ComPtrT = Microsoft::WRL::ComPtr<T>;

static void CopyToUpload_(ID3D12Resource *res, const void *src, size_t size) {
  void *mapped = nullptr;
  res->Map(0, nullptr, &mapped);
  std::memcpy(mapped, src, size);
  res->Unmap(0, nullptr);
}

ComPtrT<ID3D12Resource> Model::CreateUploadBuffer(size_t size) {
  ComPtrT<ID3D12Resource> resource;

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  HRESULT hr = dx_->GetDevice()->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));
  return resource;
}

#include <Windows.h>
#include <string>

static void ModelFatal_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Model Fatal", MB_OK | MB_ICONERROR);
  std::terminate();
}

static void ModelCheckHR_(HRESULT hr, const char *where) {
  if (FAILED(hr)) {
    char buf[512];
    sprintf_s(buf, "[Model] HRESULT failed at %s (hr=0x%08X)", where,
              (unsigned)hr);
    ModelFatal_(buf);
  }
}

bool Model::Initialize(const CreateInfo &ci) {
  dx_ = ci.dx;
  pipeline_ = ci.pipeline;

  if (!dx_ || !pipeline_) {
    ModelFatal_("[Model] dx_ or pipeline_ is null");
  }

  vertexCount_ = static_cast<uint32_t>(ci.modelData.vertices.size());
  if (vertexCount_ == 0) {
    ModelFatal_(
        "[Model] modelData.vertices is empty. OBJ load failed or path wrong.");
  }

  const size_t vbSize = sizeof(VertexData) * ci.modelData.vertices.size();

  vb_ = CreateUploadBuffer(vbSize);
  if (!vb_) {
    ModelFatal_("[Model] vb_ CreateUploadBuffer failed");
  }

  // CopyToUpload_ が Map を含むなら、そこで hr を見てない可能性があるので、
  // CopyToUpload_ 内も HRESULT チェックにするのが理想。
  CopyToUpload_(vb_.Get(), ci.modelData.vertices.data(), vbSize);

  vbv_.BufferLocation = vb_->GetGPUVirtualAddress();
  vbv_.StrideInBytes = sizeof(VertexData);
  vbv_.SizeInBytes = static_cast<UINT>(vbSize);

  cbMaterial_ = CreateUploadBuffer(sizeof(ModelMaterialCB));
  if (!cbMaterial_) {
    ModelFatal_("[Model] cbMaterial_ CreateUploadBuffer failed");
  }

  HRESULT hr =
      cbMaterial_->Map(0, nullptr, reinterpret_cast<void **>(&cbMatMapped_));
  ModelCheckHR_(hr, "cbMaterial_->Map");
  if (!cbMatMapped_) {
    ModelFatal_("[Model] cbMatMapped_ is null after Map");
  }

  cbMatMapped_->color = ci.baseColor;
  cbMatMapped_->enableLighting = ci.lightingMode;
  cbMatMapped_->uvTransform = MakeIdentity4x4();

  cbTransform_ = CreateUploadBuffer(sizeof(ModelTransformCB));
  if (!cbTransform_) {
    ModelFatal_("[Model] cbTransform_ CreateUploadBuffer failed");
  }

  hr =
      cbTransform_->Map(0, nullptr, reinterpret_cast<void **>(&cbTransMapped_));
  ModelCheckHR_(hr, "cbTransform_->Map");
  if (!cbTransMapped_) {
    ModelFatal_("[Model] cbTransMapped_ is null after Map");
  }

  cbTransMapped_->World = MakeIdentity4x4();
  cbTransMapped_->WVP = MakeIdentity4x4();

  // Texture
  if (!ci.modelData.material.textureFilePath.empty()) {
    texture_ = TextureManager::GetInstance()->Load(
        ci.modelData.material.textureFilePath);
  } else {
    texture_ = TextureManager::GetInstance()->Load("resources/white1x1.png");
  }

  if (!texture_) {
    ModelFatal_(
        "[Model] texture load failed (TextureManager::Load returned null)");
  }

  texSrvHandleGPU_ = texture_->GetSrvGpu();

  return true;
}

void Model::SetWorldTransform(const Matrix4x4 &world) { world_ = world; }

void Model::SetColor(const Vector4 &color) {
  if (cbMatMapped_) {
    cbMatMapped_->color = color;
  }
}

void Model::SetLightingMode(int32_t mode) {
  if (cbMatMapped_) {
    cbMatMapped_->enableLighting = mode;
  }
}

void Model::SetUVTransform(const Matrix4x4 &uv) {
  if (cbMatMapped_) {
    cbMatMapped_->uvTransform = uv;
  }
}

void Model::SetSpecularColor(const Vector3 &color) {
  if (cbMatMapped_) {
    cbMatMapped_->specularColor = color;
  }
}

void Model::SetShininess(float shininess) {
  if (cbMatMapped_) {
    cbMatMapped_->shininess = shininess;
  }
}

void Model::Draw(const Matrix4x4 &view, const Matrix4x4 &proj,
                 ID3D12Resource *directionalLightCB, ID3D12Resource *cameraCB,
                 ID3D12Resource *pointLightCB, ID3D12Resource *spotLightCB) {
  assert(dx_ && pipeline_);

  ID3D12GraphicsCommandList *cmd = dx_->GetCommandList();

  // Transform更新
  const Matrix4x4 wvp = Multiply(world_, Multiply(view, proj));
  cbTransMapped_->World = world_;
  cbTransMapped_->WVP = wvp;

  // worldの逆行列を計算して転置行列にする
  const Matrix4x4 worldInv = Inverse(world_);
  cbTransMapped_->WorldInverseTranspose = Transpose(worldInv);

  // PSO / RootSignature
  cmd->SetPipelineState(pipeline_->GetPipelineState());
  cmd->SetGraphicsRootSignature(pipeline_->GetRootSignature());

  // IA
  cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmd->IASetVertexBuffers(0, 1, &vbv_);

  // RootParameter index は UnifiedPipeline の生成順に合わせる
  // 0: PS Material(b0)
  // 1: VS Transform(b0)
  // 2: PS Texture Table(t0)
  // 3: PS DirectionalLight(b1)
  // 4: PS Camera(b2)
  // 5: PS PointLight(b3)
  // 6: PS SpotLight(b4)
  cmd->SetGraphicsRootConstantBufferView(0,
                                         cbMaterial_->GetGPUVirtualAddress());
  cmd->SetGraphicsRootConstantBufferView(1,
                                         cbTransform_->GetGPUVirtualAddress());

  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmd->SetDescriptorHeaps(1, heaps);
  cmd->SetGraphicsRootDescriptorTable(2, texSrvHandleGPU_);

  if (directionalLightCB) {
    cmd->SetGraphicsRootConstantBufferView(
        3, directionalLightCB->GetGPUVirtualAddress());
  }
  if (cameraCB) {
    cmd->SetGraphicsRootConstantBufferView(4, cameraCB->GetGPUVirtualAddress());
  }
  if (pointLightCB) {
    cmd->SetGraphicsRootConstantBufferView(
        5, pointLightCB->GetGPUVirtualAddress());
  }
  if (spotLightCB) {
    cmd->SetGraphicsRootConstantBufferView(
        6, spotLightCB->GetGPUVirtualAddress());
  }

  cmd->RSSetViewports(1, &dx_->GetViewport());
  cmd->RSSetScissorRects(1, &dx_->GetScissorRect());

  cmd->DrawInstanced(vertexCount_, 1, 0, 0);
}
