#define NOMINMAX
#include "Renderer.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "ParticleManager.h"
#include "particle/GPUParticleManager.h"
#include "Ring.h"
#include "Cylinder.h"
#include "ShaderCompilerUtils.h"
#include "Skybox.h"
#include "Sprite.h"
#include "SpriteResource.h"
#include "TextureResource.h"
#include "SkinCluster.h"
#include "UnifiedPipeline.h"
#include "PrimitiveDrawer.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <d3d12.h>
#include <stdexcept>

#define CHECK_INIT(call)                                                       \
  {                                                                            \
    bool _res = (call);                                                        \
    if (!_res)                                                                 \
      throw std::runtime_error(#call " failed!");                              \
  }

// シングルトンの実体を .cpp 内に隠蔽
Renderer *Renderer::GetInstance() {
  static Renderer inst;
  return &inst;
}

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::Initialize(DirectXCommon *dx) {
  assert(dx);
  dx_ = dx;

  auto *device = dx_->GetDevice();
  auto *utils = dx_->GetDXCUtils();
  auto *compiler = dx_->GetDXCCompiler();
  auto *includeHandler = dx_->GetDXCIncludeHandler();

  // 3D Pipelines
  {
    PipelineDesc desc = UnifiedPipeline::MakeObject3DDesc();
    objPipelineOpaque_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(objPipelineOpaque_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
    desc.fillMode = D3D12_FILL_MODE_WIREFRAME;
    objPipelineWireframe_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(objPipelineWireframe_->Initialize(device, utils, compiler,
                                                 includeHandler, desc));
  }

  // Skinned 3D Pipelines
  {
    PipelineDesc desc = UnifiedPipeline::MakeSkinnedObject3DDesc();
    skinnedPipelineOpaque_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(skinnedPipelineOpaque_->Initialize(device, utils, compiler,
                                                  includeHandler, desc));
    desc.fillMode = D3D12_FILL_MODE_WIREFRAME;
    skinnedPipelineWireframe_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(skinnedPipelineWireframe_->Initialize(device, utils, compiler,
                                                     includeHandler, desc));
  }

  // Skybox Pipeline
  {
    PipelineDesc desc = UnifiedPipeline::MakeSkyboxDesc();
    skyboxPipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(skyboxPipeline_->Initialize(device, utils, compiler,
                                           includeHandler, desc));
  }

  // Sprite Pipelines
  {
    PipelineDesc desc = UnifiedPipeline::MakeSpriteDesc();
    desc.blendMode = BlendMode::Alpha;
    spritePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(spritePipelineAlpha_->Initialize(device, utils, compiler,
                                                includeHandler, desc));
    desc.blendMode = BlendMode::Add;
    spritePipelineAdd_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(spritePipelineAdd_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
    desc.blendMode = BlendMode::Subtract;
    spritePipelineSub_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(spritePipelineSub_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
    desc.blendMode = BlendMode::Multiply;
    spritePipelineMul_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(spritePipelineMul_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
    desc.blendMode = BlendMode::Screen;
    spritePipelineScreen_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(spritePipelineScreen_->Initialize(device, utils, compiler,
                                                 includeHandler, desc));
  }

  // Particle Pipelines
  {
    PipelineDesc desc = UnifiedPipeline::MakeParticleDesc();
    desc.blendMode = BlendMode::Alpha;
    particlePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(particlePipelineAlpha_->Initialize(device, utils, compiler,
                                                  includeHandler, desc));
    desc.blendMode = BlendMode::Add;
    particlePipelineAdd_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(particlePipelineAdd_->Initialize(device, utils, compiler,
                                                includeHandler, desc));
    desc.blendMode = BlendMode::Subtract;
    particlePipelineSub_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(particlePipelineSub_->Initialize(device, utils, compiler,
                                                includeHandler, desc));
    desc.blendMode = BlendMode::Multiply;
    particlePipelineMul_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(particlePipelineMul_->Initialize(device, utils, compiler,
                                                includeHandler, desc));
    desc.blendMode = BlendMode::Screen;
    particlePipelineScreen_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(particlePipelineScreen_->Initialize(device, utils, compiler,
                                                   includeHandler, desc));
  }

  // Primitive Pipeline
  {
    PipelineDesc desc = UnifiedPipeline::MakePrimitiveDesc();
    primitivePipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(primitivePipeline_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
  }

  // Effect Pipeline
  {
    PipelineDesc desc = UnifiedPipeline::MakeUnlitEffectDesc();
    effectPipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(effectPipeline_->Initialize(device, utils, compiler,
                                           includeHandler, desc));
    PipelineDesc ringDesc = UnifiedPipeline::MakeRingDesc();
    ringPipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(ringPipeline_->Initialize(device, utils, compiler,
                                         includeHandler, ringDesc));
    PipelineDesc cylinderDesc = UnifiedPipeline::MakeCylinderDesc();
    cylinderPipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(cylinderPipeline_->Initialize(device, utils, compiler,
                                             includeHandler, cylinderDesc));
  }

  // CopyImage Pipeline
  {
    PipelineDesc desc = UnifiedPipeline::MakeCopyImageDesc();
    copyImagePipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(copyImagePipeline_->Initialize(device, utils, compiler,
                                              includeHandler, desc));
    
    PipelineDesc grayDesc = UnifiedPipeline::MakeGrayscaleDesc();
    grayscalePipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(grayscalePipeline_->Initialize(device, utils, compiler,
                                               includeHandler, grayDesc));

    PipelineDesc sepiaDesc = UnifiedPipeline::MakeSepiaDesc();
    sepiaPipeline_ = std::make_unique<UnifiedPipeline>();
    CHECK_INIT(sepiaPipeline_->Initialize(device, utils, compiler,
                                             includeHandler, sepiaDesc));
  }

  // Primitive Drawer
  primitiveDrawer_ = std::make_unique<PrimitiveDrawer>();
  primitiveDrawer_->Initialize(dx_);

  auto align256 = [](size_t size) -> size_t { return (size + 255) & ~255; };

  cameraCB_ = CreateUploadBuffer(align256(sizeof(CameraForGPU)));
  cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraMapped_));
  directionalLightCB_ =
      CreateUploadBuffer(align256(sizeof(DirectionalLightGroupCB)));
  directionalLightCB_->Map(0, nullptr,
                           reinterpret_cast<void **>(&directionalLightMapped_));
  pointLightCB_ = CreateUploadBuffer(align256(sizeof(PointLightGroupCB)));
  pointLightCB_->Map(0, nullptr, reinterpret_cast<void **>(&pointLightMapped_));
  spotLightCB_ = CreateUploadBuffer(align256(sizeof(SpotLightGroupCB)));
  spotLightCB_->Map(0, nullptr, reinterpret_cast<void **>(&spotLightMapped_));

  skinningInformationCB_ = CreateUploadBuffer(align256(sizeof(SkinningInformation)));
  skinningInformationCB_->Map(0, nullptr, reinterpret_cast<void **>(&skinningInformationMapped_));

  GPUParticleManager::GetInstance()->Initialize(dx_);

  // Primitive用定数バッファ
  primitiveTransformCB_ = CreateUploadBuffer(sizeof(TransformCB));
  primitiveTransformCB_->Map(
      0, nullptr, reinterpret_cast<void **>(&primitiveTransformMapped_));

  InitSkinningPipeline_();
}

void Renderer::InitSkinningPipeline_() {
  auto* device = dx_->GetDevice();

  // Root Signature
  D3D12_DESCRIPTOR_RANGE ranges[4]{};
  // t0: Palette
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 1;
  ranges[0].BaseShaderRegister = 0;
  // t1: InputVertices
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[1].NumDescriptors = 1;
  ranges[1].BaseShaderRegister = 1;
  // t2: Influences
  ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[2].NumDescriptors = 1;
  ranges[2].BaseShaderRegister = 2;
  // u0: OutputVertices
  ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[3].NumDescriptors = 1;
  ranges[3].BaseShaderRegister = 0;

  D3D12_ROOT_PARAMETER params[5]{};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[0].DescriptorTable.NumDescriptorRanges = 1;
  params[0].DescriptorTable.pDescriptorRanges = &ranges[0];
  params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[1].DescriptorTable.NumDescriptorRanges = 1;
  params[1].DescriptorTable.pDescriptorRanges = &ranges[1];
  params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[2].DescriptorTable.NumDescriptorRanges = 1;
  params[2].DescriptorTable.pDescriptorRanges = &ranges[2];
  params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[3].DescriptorTable.NumDescriptorRanges = 1;
  params[3].DescriptorTable.pDescriptorRanges = &ranges[3];
  params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  params[4].Descriptor.ShaderRegister = 0;
  params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_ROOT_SIGNATURE_DESC rsDesc{};
  rsDesc.NumParameters = 5;
  rsDesc.pParameters = params;
  rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

  ComPtr<ID3DBlob> blob, err;
  D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);
  if (err) {
    OutputDebugStringA((const char*)err->GetBufferPointer());
  }
  device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&skinningRootSignature_));

  // Pipeline State
  ComPtr<IDxcBlob> csBlob = CompileShader(L"resources/engine/shaders/Skinning.CS.hlsl", L"cs_6_0", dx_->GetDXCUtils(), dx_->GetDXCCompiler(), dx_->GetDXCIncludeHandler());
  assert(csBlob);

  D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = skinningRootSignature_.Get();
  psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
  psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

  device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&skinningPipelineState_));
}

void Renderer::DispatchSkinning(ModelInstance* instance) {
  if (!instance || !instance->GetResource() || !instance->GetSkinCluster()) return;
  auto* resource = instance->GetResource();
  auto* skinCluster = instance->GetSkinCluster();
  auto* cmdList = dx_->GetCommandList();

  // 1. バリア設定 (VertexBuffer -> UAV)
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = skinCluster->skinnedVertexBuffer.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  // 2. パイプラインセット
  cmdList->SetComputeRootSignature(skinningRootSignature_.Get());
  cmdList->SetPipelineState(skinningPipelineState_.Get());

  // 3. ディスクリプタセット
  ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
  cmdList->SetDescriptorHeaps(1, heaps);

  auto& srvAlloc = dx_->GetSrvAllocator();
  // t0: Palette
  cmdList->SetComputeRootDescriptorTable(0, srvAlloc.Gpu(skinCluster->srvIndex));
  // t1: InputVertices
  cmdList->SetComputeRootDescriptorTable(1, srvAlloc.Gpu(resource->GetVertexSRVIndex()));
  // t2: Influences
  cmdList->SetComputeRootDescriptorTable(2, srvAlloc.Gpu(resource->GetBoneSRVIndex()));
  // u0: OutputVertices
  cmdList->SetComputeRootDescriptorTable(3, srvAlloc.Gpu(skinCluster->uavIndex));
  // b0: SkinningInformation
  skinningInformationMapped_->numVertices = resource->GetVertexCount();
  cmdList->SetComputeRootConstantBufferView(4, skinningInformationCB_->GetGPUVirtualAddress());

  // 4. Dispatch
  uint32_t numVertices = resource->GetVertexCount();
  cmdList->Dispatch((numVertices + 1023) / 1024, 1, 1);

  // 5. バリア設定 (UAV -> VertexBuffer)
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  cmdList->ResourceBarrier(1, &barrier);
}

void Renderer::SetCamera(const Camera &camera) {
  view_ = camera.GetViewMatrix();
  proj_ = camera.GetProjectionMatrix();
  if (cameraMapped_) {
    Matrix4x4 invView = Inverse(view_);
    cameraMapped_->worldPosition = {invView.m[3][0], invView.m[3][1],
                                    invView.m[3][2]};
    cameraMapped_->pad = 0.0f;
  }
}

void Renderer::SetDirectionalLights(const std::vector<DirLight> &lights,
                                    bool groupEnabled) {
  if (!directionalLightMapped_)
    return;
  std::memset(directionalLightMapped_, 0, sizeof(DirectionalLightGroupCB));
  int count = static_cast<int>(
      std::min(lights.size(), static_cast<size_t>(kMaxDirLights)));
  directionalLightMapped_->count = count;
  directionalLightMapped_->enabled = groupEnabled ? 1 : 0;
  for (int i = 0; i < count; ++i) {
    const auto &src = lights[i];
    auto &dst = directionalLightMapped_->lights[i];
    dst.color[0] = src.color.x;
    dst.color[1] = src.color.y;
    dst.color[2] = src.color.z;
    dst.color[3] = 1.0f;
    dst.direction[0] = src.direction.x;
    dst.direction[1] = src.direction.y;
    dst.direction[2] = src.direction.z;
    dst.intensity = src.intensity;
    dst.enabled = src.enabled ? 1 : 0;
  }
}

void Renderer::SetPointLights(const std::vector<PointLight> &lights,
                              bool groupEnabled) {
  if (!pointLightMapped_)
    return;
  std::memset(pointLightMapped_, 0, sizeof(PointLightGroupCB));
  int count = static_cast<int>(
      std::min(lights.size(), static_cast<size_t>(kMaxPointLights)));
  pointLightMapped_->count = count;
  pointLightMapped_->enabled = groupEnabled ? 1 : 0;
  for (int i = 0; i < count; ++i) {
    const auto &src = lights[i];
    auto &dst = pointLightMapped_->lights[i];
    dst.color[0] = src.color.x;
    dst.color[1] = src.color.y;
    dst.color[2] = src.color.z;
    dst.color[3] = 1.0f;
    dst.position[0] = src.position.x;
    dst.position[1] = src.position.y;
    dst.position[2] = src.position.z;
    dst.intensity = src.intensity;
    dst.radius = src.radius;
    dst.decay = src.decay;
    dst.enabled = src.enabled ? 1 : 0;
  }
}

void Renderer::SetSpotLights(const std::vector<SpotLight> &lights,
                             bool groupEnabled) {
  if (!spotLightMapped_)
    return;
  std::memset(spotLightMapped_, 0, sizeof(SpotLightGroupCB));
  int count = static_cast<int>(
      std::min(lights.size(), static_cast<size_t>(kMaxSpotLights)));
  spotLightMapped_->count = count;
  spotLightMapped_->enabled = groupEnabled ? 1 : 0;
  static constexpr float kPi = 3.14159265f;
  for (int i = 0; i < count; ++i) {
    const auto &src = lights[i];
    auto &dst = spotLightMapped_->lights[i];
    dst.color[0] = src.color.x;
    dst.color[1] = src.color.y;
    dst.color[2] = src.color.z;
    dst.color[3] = 1.0f;
    dst.position[0] = src.position.x;
    dst.position[1] = src.position.y;
    dst.position[2] = src.position.z;
    dst.direction[0] = src.direction.x;
    dst.direction[1] = src.direction.y;
    dst.direction[2] = src.direction.z;
    dst.intensity = src.intensity;
    dst.distance = src.distance;
    dst.decay = src.decay;
    dst.cosAngle = std::cos(src.coneAngleDeg * (kPi / 180.0f));
    dst.enabled = src.enabled ? 1 : 0;
  }
}

float Renderer::GetScreenWidth() const { return dx_->GetViewport().Width; }
float Renderer::GetScreenHeight() const { return dx_->GetViewport().Height; }
float Renderer::GetAspectRatio() const {
  float h = GetScreenHeight();
  return (h != 0.0f) ? (GetScreenWidth() / h) : 1.0f;
}

void Renderer::DrawModel(ModelInstance *instance) {
  if (!instance || !instance->GetResource())
    return;
  auto *cmdList = dx_->GetCommandList();
  auto *resource = instance->GetResource();

  // 描画直前に WVP 行列を最終計算して GPU に送る
  auto *cbTrans = instance->GetTransformMapped();
  if (cbTrans) {
    // WVP = (World * View) * Projection
    Matrix4x4 worldView = Multiply(instance->GetWorld(), view_);
    cbTrans->WVP = Multiply(worldView, proj_);
  }

  // 1. パイプライン設定
  bool hasBones = resource->HasBones();
  SkinCluster* skinCluster = instance->GetSkinCluster();

  UnifiedPipeline *pipeline = nullptr;
  if (hasBones && skinCluster) {
    // CSでスキニング済みのため、非スキニング用パイプラインを使用する
    pipeline = instance->IsWireframe() ? objPipelineWireframe_.get()
                                       : objPipelineOpaque_.get();
  } else {
    pipeline = instance->IsWireframe() ? objPipelineWireframe_.get()
                                       : objPipelineOpaque_.get();
  }
  pipeline->SetPipelineState(cmdList);

  // 2. 頂点バッファ設定
  D3D12_VERTEX_BUFFER_VIEW vbv{};
  if (hasBones && skinCluster) {
    // スキニング済みバッファを使用
    vbv = skinCluster->vbView;
  } else {
    // 静的バッファを使用
    vbv.BufferLocation = resource->GetVBVAddress();
    vbv.SizeInBytes = resource->GetVBVSize();
    vbv.StrideInBytes = resource->GetVBVStride();
  }
  cmdList->IASetVertexBuffers(0, 1, &vbv);
  
  // Index Buffer
  if (resource->GetIndexCount() > 0) {
    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = resource->GetIBVAddress();
    ibv.Format = DXGI_FORMAT_R32_UINT;
    ibv.SizeInBytes = resource->GetIBVSize();
    cmdList->IASetIndexBuffer(&ibv);
  }

  // 3. 定数バッファ (Material / Transform)
  cmdList->SetGraphicsRootConstantBufferView(0,
                                             instance->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1,
                                             instance->GetTransformCBAddress());

  // 4. テクスチャ設定
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);

  // 通常テクスチャ
  D3D12_GPU_DESCRIPTOR_HANDLE texHandle{};
  texHandle.ptr = resource->GetTextureHandleGPUAsUInt64();
  cmdList->SetGraphicsRootDescriptorTable(2, texHandle);

  // t1: 環境マップ 
  if (environmentMap_) {
    cmdList->SetGraphicsRootDescriptorTable(7, environmentMap_->GetSrvGpu());
  }

  // 5. ライト・カメラ設定
  cmdList->SetGraphicsRootConstantBufferView(
      3, directionalLightCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(4,
                                             cameraCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(
      5, pointLightCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(
      6, spotLightCB_->GetGPUVirtualAddress());


  // 6. 描画
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  if (resource->GetIndexCount() > 0) {
    cmdList->DrawIndexedInstanced(resource->GetIndexCount(), 1, 0, 0, 0);
  } else {
    cmdList->DrawInstanced(resource->GetVertexCount(), 1, 0, 0);
  }
}

void Renderer::DrawSprite(Sprite *sprite) {
  if (!sprite || !sprite->GetResource())
    return;
  auto *cmdList = dx_->GetCommandList();
  auto *res = sprite->GetResource();

  // WVP行列の合成 (Spriteは通常、カメラのViewを無視してProjectionのみ掛ける)
  auto *cbTrans = sprite->GetTransformMapped();
  if (cbTrans) {
    cbTrans->WVP = Multiply(sprite->GetWorldMatrix(), proj_);
  }

  UnifiedPipeline *pipeline = GetSpritePipeline_(sprite->GetBlendMode());
  pipeline->SetPipelineState(cmdList);

  // 頂点・インデックスバッファ
  D3D12_VERTEX_BUFFER_VIEW vbv{};
  vbv.BufferLocation = res->GetVBAddress();
  vbv.SizeInBytes = res->GetVBSize();
  vbv.StrideInBytes = res->GetVBStride();
  cmdList->IASetVertexBuffers(0, 1, &vbv);

  D3D12_INDEX_BUFFER_VIEW ibv{};
  ibv.BufferLocation = res->GetIBAddress();
  ibv.SizeInBytes = res->GetIBSize();
  ibv.Format = DXGI_FORMAT_R16_UINT;
  cmdList->IASetIndexBuffer(&ibv);

  // 定数バッファ
  cmdList->SetGraphicsRootConstantBufferView(0, sprite->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1,
                                             sprite->GetTransformCBAddress());

  // テクスチャ
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);
  cmdList->SetGraphicsRootDescriptorTable(2, sprite->GetTextureHandle());

  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdList->DrawIndexedInstanced(res->GetIndexCount(), 1, 0, 0, 0);
}

void Renderer::DrawSkybox(Skybox *skybox) {
  if (!skybox)
    return;
  auto *cmdList = dx_->GetCommandList();
  skyboxPipeline_->SetPipelineState(cmdList);

  D3D12_VERTEX_BUFFER_VIEW vbv{};
  vbv.BufferLocation = skybox->GetVBAddress();
  vbv.SizeInBytes = skybox->GetVBSize();
  vbv.StrideInBytes = skybox->GetVBStride();
  cmdList->IASetVertexBuffers(0, 1, &vbv);

  D3D12_INDEX_BUFFER_VIEW ibv{};
  ibv.BufferLocation = skybox->GetIBAddress();
  ibv.SizeInBytes = skybox->GetIBSize();
  ibv.Format = DXGI_FORMAT_R32_UINT;
  cmdList->IASetIndexBuffer(&ibv);

  cmdList->SetGraphicsRootConstantBufferView(0, skybox->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1,
                                             skybox->GetTransformCBAddress());

  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);
  cmdList->SetGraphicsRootDescriptorTable(2, skybox->GetTexture()->GetSrvGpu());

  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdList->DrawIndexedInstanced(skybox->GetIndexCount(), 1, 0, 0, 0);
}

void Renderer::DrawParticles(ParticleManager *pm, BlendMode blendMode) {
  if (!pm)
    return;
  auto *cmdList = dx_->GetCommandList();
  UnifiedPipeline *pipeline = GetParticlePipeline_(blendMode);
  pipeline->SetPipelineState(cmdList);
  pm->DrawInternal(cmdList);
}

void Renderer::RenderPrimitives() {
  auto *cmdList = dx_->GetCommandList();

  // WVP計算
  if (primitiveTransformMapped_) {
    // Primitiveは基本的にワールド座標で指定されるため、World行列は単位行列
    primitiveTransformMapped_->WVP = Multiply(view_, proj_);
  }

  primitivePipeline_->SetPipelineState(cmdList);
  // 他のパイプラインと共通の構造（1番をTransform）にする
  cmdList->SetGraphicsRootConstantBufferView(
      1, primitiveTransformCB_->GetGPUVirtualAddress());

  primitiveDrawer_->Draw(cmdList);
  primitiveDrawer_->Reset(); // 描画後にリセット
}

void Renderer::DrawEffectModel(ModelInstance *instance) {
  if (!instance || !instance->GetResource())
    return;
  auto *cmdList = dx_->GetCommandList();
  auto *resource = instance->GetResource();

  // WVP 行列の計算と転送
  auto *cbTrans = instance->GetTransformMapped();
  if (cbTrans) {
    Matrix4x4 worldView = Multiply(instance->GetWorld(), view_);
    cbTrans->WVP = Multiply(worldView, proj_);
    cbTrans->World = instance->GetWorld();
    cbTrans->WorldInverseTranspose = Transpose(Inverse(instance->GetWorld()));
  }

  // エフェクト用パイプラインをセット
  effectPipeline_->SetPipelineState(cmdList);

  // 頂点バッファ
  D3D12_VERTEX_BUFFER_VIEW vbv{};
  vbv.BufferLocation = resource->GetVBVAddress();
  vbv.SizeInBytes = resource->GetVBVSize();
  vbv.StrideInBytes = resource->GetVBVStride();
  cmdList->IASetVertexBuffers(0, 1, &vbv);

  // Index Buffer
  if (resource->GetIndexCount() > 0) {
    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = resource->GetIBVAddress();
    ibv.Format = DXGI_FORMAT_R32_UINT;
    ibv.SizeInBytes = resource->GetIBVSize();
    cmdList->IASetIndexBuffer(&ibv);
  }

  // 定数バッファ (0:マテリアル, 1:トランスフォーム)
  cmdList->SetGraphicsRootConstantBufferView(0,
                                             instance->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1,
                                             instance->GetTransformCBAddress());

  // テクスチャ
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);
  D3D12_GPU_DESCRIPTOR_HANDLE texHandle{};
  texHandle.ptr = resource->GetTextureHandleGPUAsUInt64();
  cmdList->SetGraphicsRootDescriptorTable(2, texHandle);

  // ライト・カメラ (シェーダがこれらを参照しているため、Unlitでもセットが必要)
  cmdList->SetGraphicsRootConstantBufferView(
      3, directionalLightCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(4,
                                             cameraCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(
      5, pointLightCB_->GetGPUVirtualAddress());
  cmdList->SetGraphicsRootConstantBufferView(
      6, spotLightCB_->GetGPUVirtualAddress());

  // 描画実行
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  if (resource->GetIndexCount() > 0) {
    cmdList->DrawIndexedInstanced(resource->GetIndexCount(), 1, 0, 0, 0);
  } else {
    cmdList->DrawInstanced(resource->GetVertexCount(), 1, 0, 0);
  }
}

void Renderer::DrawRing(Ring *ring, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
  if (!ring || !textureHandle.ptr)
    return;
  auto *cmdList = dx_->GetCommandList();

  // パイプライン設定
  ringPipeline_->SetPipelineState(cmdList);

  // 定数バッファ (0:マテリアル, 1:トランスフォーム)
  cmdList->SetGraphicsRootConstantBufferView(0, ring->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1, ring->GetTransformCBAddress());

  // テクスチャ
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);
  cmdList->SetGraphicsRootDescriptorTable(2, textureHandle);

  // 描画
  ring->Draw(cmdList);
}

void Renderer::DrawCylinder(Cylinder *cylinder, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
  if (!cylinder || !textureHandle.ptr)
    return;
  auto *cmdList = dx_->GetCommandList();

  // パイプライン設定
  cylinderPipeline_->SetPipelineState(cmdList);

  // 定数バッファ (0:マテリアル, 1:トランスフォーム)
  cmdList->SetGraphicsRootConstantBufferView(0, cylinder->GetMaterialCBAddress());
  cmdList->SetGraphicsRootConstantBufferView(1, cylinder->GetTransformCBAddress());

  // テクスチャ
  ID3D12DescriptorHeap *heaps[] = {dx_->GetSRVHeap()};
  cmdList->SetDescriptorHeaps(1, heaps);
  cmdList->SetGraphicsRootDescriptorTable(2, textureHandle);

  // 描画
  cylinder->Draw(cmdList);
}

UnifiedPipeline *Renderer::GetSpritePipeline_(BlendMode mode) {
  switch (mode) {
  case BlendMode::Add:
    return spritePipelineAdd_.get();
  case BlendMode::Subtract:
    return spritePipelineSub_.get();
  case BlendMode::Multiply:
    return spritePipelineMul_.get();
  case BlendMode::Screen:
    return spritePipelineScreen_.get();
  default:
    return spritePipelineAlpha_.get();
  }
}

UnifiedPipeline *Renderer::GetParticlePipeline_(BlendMode mode) {
  switch (mode) {
  case BlendMode::Add:
    return particlePipelineAdd_.get();
  case BlendMode::Subtract:
    return particlePipelineSub_.get();
  case BlendMode::Multiply:
    return particlePipelineMul_.get();
  case BlendMode::Screen:
    return particlePipelineScreen_.get();
  default:
    return particlePipelineAlpha_.get();
  }
}

Microsoft::WRL::ComPtr<ID3D12Resource>
Renderer::CreateUploadBuffer(size_t size) {
  auto device = dx_->GetDevice();
  Microsoft::WRL::ComPtr<ID3D12Resource> res;
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
  device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                  IID_PPV_ARGS(&res));
  return res;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::CreateBuffer(size_t size) {
  return CreateUploadBuffer(size);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::CreateUAVBuffer(size_t size) {
  auto device = dx_->GetDevice();
  Microsoft::WRL::ComPtr<ID3D12Resource> res;
  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  HRESULT hr = device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&res));
  if (FAILED(hr)) {
    return nullptr;
  }
  return res;
}

// 描画予約をDrawerへ流す
void Renderer::DrawGPUParticles() {
  GPUParticleManager::GetInstance()->Update();
  GPUParticleManager::GetInstance()->Draw();
}

void Renderer::DrawLine(const Vector3 &start, const Vector3 &end,
                        const Vector4 &color) {
  primitiveDrawer_->AddLine(start, end, color);
}

void Renderer::DrawGrid(float size, int divisions, const Vector4 &color) {
  primitiveDrawer_->AddGrid(size, divisions, color);
}

void Renderer::DrawFullscreen(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, PostProcessMode mode) {
  UnifiedPipeline *pipeline = nullptr;
  switch (mode) {
  case PostProcessMode::Normal:
    pipeline = copyImagePipeline_.get();
    break;
  case PostProcessMode::Grayscale:
    pipeline = grayscalePipeline_.get();
    break;
  case PostProcessMode::Sepia:
    pipeline = sepiaPipeline_.get();
    break;
  }

  if (!pipeline)
    return;

  auto *cmdList = dx_->GetCommandList();

  // パイプライン設定
  pipeline->SetPipelineState(cmdList);

  // テクスチャをセット (t0)
  cmdList->SetGraphicsRootDescriptorTable(0, textureHandle);

  // 頂点バッファなしで3頂点描画（大きな三角形1つで全画面を覆う）
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdList->DrawInstanced(3, 1, 0, 0);
}