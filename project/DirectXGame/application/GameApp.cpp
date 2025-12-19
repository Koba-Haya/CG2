#define NOMINMAX
#include "GameApp.h"
#include "DebugCamera.h"
#include "DirectXResourceUtils.h"
#include "ModelUtils.h"
#include "TextureUtils.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <objbase.h>
#include <random>

namespace {
std::mt19937 &GetRNG() {
  static std::mt19937 rng{std::random_device{}()};
  return rng;
}
float Rand01() {
  static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  return dist(GetRNG());
}
float RandRange(float minV, float maxV) { return minV + (maxV - minV) * Rand01(); }

Vector3 HSVtoRGB(const Vector3 &hsv) {
  float h = hsv.x;
  float s = hsv.y;
  float v = hsv.z;

  if (s <= 0.0f) {
    return {v, v, v};
  }

  h = std::fmod(h, 1.0f);
  if (h < 0.0f) {
    h += 1.0f;
  }

  float hf = h * 6.0f;
  int i = static_cast<int>(std::floor(hf));
  float f = hf - static_cast<float>(i);

  float p = v * (1.0f - s);
  float q = v * (1.0f - s * f);
  float t = v * (1.0f - s * (1.0f - f));

  switch (i % 6) {
  case 0: return {v, t, p};
  case 1: return {q, v, p};
  case 2: return {p, v, t};
  case 3: return {p, q, v};
  case 4: return {t, p, v};
  default: return {v, p, q};
  }
}
} // namespace

GameApp::GameApp()
    : winApp_(), dx_(), input_(), audio_(), transform_{}, cameraTransform_{},
      transformSprite_{}, uvTransformSprite_{}, transform2_{}, particles_{} {}

GameApp::~GameApp() { Finalize(); }

bool GameApp::Initialize() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  assert(SUCCEEDED(hr));

  InitLogging_();

  winApp_.Initialize();

  DirectXCommon::InitParams params{
      winApp_.GetHInstance(),
      winApp_.GetHwnd(),
      WinApp::kClientWidth,
      WinApp::kClientHeight,
  };
  dx_.Initialize(params);

  // ImGui初期化
  imgui_.Initialize(&winApp_, &dx_);

  bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
  assert(inputOk);

  bool audioOk = audio_.Initialize();
  assert(audioOk);

  shaderCompiler_.Initialize(dx_.GetDXCUtils(), dx_.GetDXCCompiler(), dx_.GetDXCIncludeHandler());

  InitPipelines_();
  InitResources_();

  debugCamera_ = std::make_unique<DebugCamera>();
  InitCamera_();

  accelerationField_.acceleration = {15.0f, 0.0f, 0.0f};
  accelerationField_.area.min = {-1.0f, -1.0f, -1.0f};
  accelerationField_.area.max = {1.0f, 1.0f, 1.0f};

  return true;
}

int GameApp::Run() {
  if (!Initialize()) {
    return -1;
  }

  while (winApp_.ProcessMessage()) {
    input_.Update();
    Update();
    Draw();
  }

  Finalize();
  return 0;
}

void GameApp::Finalize() {
  static bool finalized = false;
  if (finalized) {
    return;
  }
  finalized = true;

  // リソース解放
  imgui_.Finalize();

  audio_.Shutdown();
  input_.Finalize();

  winApp_.Finalize();
  CoUninitialize();
}

void GameApp::Update() {
  if (debugCamera_) {
    debugCamera_->Update(input_);
    view3D_ = debugCamera_->GetViewMatrix();
  } else {
    view3D_ = Inverse(MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
                                       {0.0f, 0.0f, -10.0f}));
  }

  proj3D_ = MakePerspectiveFovMatrix(
      0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

#ifdef USE_IMGUI
  static bool settingsOpen = true;
  imgui_.Begin();
  if (settingsOpen) {
    ImGui::Begin("Settings", &settingsOpen);

    ImGui::Text("Particles");
    ImGui::SliderInt("最大発生数", reinterpret_cast<int *>(&particleCountUI_), 0,
                     static_cast<int>(kParticleCount_));

    ImGui::SliderFloat("発生頻度（1秒あたりの発生数）", &particleEmitRate_, 0.0f, 100.0f);
    if (particleEmitRate_ < 0.0f) {
      particleEmitRate_ = 0.0f;
    }

    ImGui::DragFloat3("エミッタ位置", reinterpret_cast<float *>(&particleSpawnCenter_), 0.01f);
    ImGui::DragFloat3("エミッタ範囲", reinterpret_cast<float *>(&particleSpawnExtent_), 0.01f, 0.0f, 10.0f);

    ImGui::DragFloat3("初速度", reinterpret_cast<float *>(&particleBaseDir_), 0.01f, -1.0f, 1.0f);
    ImGui::SliderFloat("初速度ランダム範囲", &particleDirRandomness_, 0.0f, 1.0f);

    ImGui::SliderFloat("最低速度", &particleSpeedMin_, 0.0f, 10.0f);
    ImGui::SliderFloat("最大速度", &particleSpeedMax_, 0.0f, 10.0f);
    if (particleSpeedMin_ > particleSpeedMax_) {
      particleSpeedMin_ = particleSpeedMax_;
    }

    ImGui::SliderFloat("最低寿命", &particleLifeMin_, 0.1f, 10.0f);
    ImGui::SliderFloat("最大寿命", &particleLifeMax_, 0.1f, 10.0f);
    if (particleLifeMin_ > particleLifeMax_) {
      particleLifeMin_ = particleLifeMax_;
    }

    ImGui::Checkbox("Emitter範囲を表示", &showEmitterGizmo_);

    int shapeIndex = (emitterShape_ == EmitterShape::Box) ? 0 : 1;
    const char *shapeItems[] = {"Box(AABB)", "Sphere"};
    ImGui::Combo("Emitter形状", &shapeIndex, shapeItems, IM_ARRAYSIZE(shapeItems));
    emitterShape_ = (shapeIndex == 0) ? EmitterShape::Box : EmitterShape::Sphere;

    ImGui::Separator();
    int colorModeIndex = static_cast<int>(particleColorMode_);
    const char *colorModeItems[] = {"Random RGB", "Range RGB", "Range HSV", "Fixed Color"};
    ImGui::Combo("Mode", &colorModeIndex, colorModeItems, IM_ARRAYSIZE(colorModeItems));
    particleColorMode_ = static_cast<ParticleColorMode>(colorModeIndex);

    ImGui::ColorEdit4("Base Color", reinterpret_cast<float *>(&particleBaseColor_));

    if (particleColorMode_ == ParticleColorMode::RangeRGB) {
      ImGui::SliderFloat3("RGB Range", reinterpret_cast<float *>(&particleColorRangeRGB_), 0.0f, 1.0f);
    }

    if (particleColorMode_ == ParticleColorMode::RangeHSV) {
      ImGui::SliderFloat3("Base HSV", reinterpret_cast<float *>(&particleBaseHSV_), 0.0f, 1.0f);
      ImGui::SliderFloat3("HSV Range", reinterpret_cast<float *>(&particleHSVRange_), 0.0f, 1.0f);
    }

    ImGui::Text("Blend");
    const char *particleBlendItems[] = {"Alpha (通常)", "Add (加算)", "Subtract (減算)", "Multiply (乗算)", "Screen (スクリーン)"};
    ImGui::Combo("Particle Blend", &particleBlendMode_, particleBlendItems, IM_ARRAYSIZE(particleBlendItems));

    ImGui::Text("Acceleration Field");
    ImGui::Checkbox("加速度場を有効化", &enableAccelerationField_);
    ImGui::DragFloat3("加速度", reinterpret_cast<float *>(&accelerationField_.acceleration), 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat3("範囲Min", reinterpret_cast<float *>(&accelerationField_.area.min), 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat3("範囲Max", reinterpret_cast<float *>(&accelerationField_.area.max), 0.1f, -10.0f, 10.0f);

    ImGui::End();
  }
  imgui_.End();
#endif

  if (particleMatrices_) {
    float deltaTime = 1.0f / 60.0f;
    uint32_t capacity = std::min(particleCountUI_, kParticleCount_);

    uint32_t gpuIndex = 0;
    for (auto it = particles_.begin(); it != particles_.end();) {
      Particle &p = *it;

      p.age += deltaTime;
      if (p.age >= p.lifetime) {
        it = particles_.erase(it);
        continue;
      }

      if (enableAccelerationField_) {
        if (IsCollision(accelerationField_.area, p.transform.translate)) {
          p.velocity.x += accelerationField_.acceleration.x * deltaTime;
          p.velocity.y += accelerationField_.acceleration.y * deltaTime;
          p.velocity.z += accelerationField_.acceleration.z * deltaTime;
        }
      }

      p.transform.translate.x += p.velocity.x * deltaTime;
      p.transform.translate.y += p.velocity.y * deltaTime;
      p.transform.translate.z += p.velocity.z * deltaTime;

      float t = p.age / p.lifetime;
      float alpha = std::clamp(1.0f - t, 0.0f, 1.0f);

      if (gpuIndex < capacity) {
        Matrix4x4 world = MakeBillboardMatrix(p.transform.scale, p.transform.translate, view3D_);
        Matrix4x4 wvp = Multiply(world, Multiply(view3D_, proj3D_));
        particleMatrices_[gpuIndex].World = world;
        particleMatrices_[gpuIndex].WVP = wvp;
        particleMatrices_[gpuIndex].color = {p.color.x, p.color.y, p.color.z, alpha};
        ++gpuIndex;
      }

      ++it;
    }

    activeParticleCount_ = gpuIndex;

    uint32_t currentCount = static_cast<uint32_t>(particles_.size());
    uint32_t maxCount = capacity;

    if (particleEmitRate_ > 0.0f && currentCount < maxCount) {
      particleEmitAccum_ += particleEmitRate_ * deltaTime;
      while (particleEmitAccum_ >= 1.0f && currentCount < maxCount) {
        Particle p{};
        RespawnParticle_(p);
        particles_.push_back(p);
        particleEmitAccum_ -= 1.0f;
        ++currentCount;
      }
    } else {
      particleEmitAccum_ = 0.0f;
    }
  }
}

void GameApp::Draw() {
  dx_.BeginFrame();
  auto *cmdList = dx_.GetCommandList();

  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      dx_.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

  UINT backBufferIndex = dx_.GetSwapChain()->GetCurrentBackBufferIndex();
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx_.GetRTVHandle(backBufferIndex);

  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  cmdList->RSSetViewports(1, &dx_.GetViewport());
  cmdList->RSSetScissorRects(1, &dx_.GetScissorRect());

  Matrix4x4 viewMatrix = view3D_;
  Matrix4x4 projectionMatrix = proj3D_;

  Matrix4x4 worldSphere = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
  model_.SetWorldTransform(worldSphere);
  model_.SetLightingMode(lightingMode_);

  Matrix4x4 worldPlane = MakeAffineMatrix(transform2_.scale, transform2_.rotate, transform2_.translate);
  planeModel_.SetWorldTransform(worldPlane);

  cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
  cmdList->SetPipelineState(objPipeline_.GetPipelineState());

  model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
  planeModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());

  if (showEmitterGizmo_) {
    if (emitterShape_ == EmitterShape::Box) {
      Vector3 scale = {particleSpawnExtent_.x * 2.0f, particleSpawnExtent_.y * 2.0f, particleSpawnExtent_.z * 2.0f};
      Matrix4x4 world = MakeAffineMatrix(scale, {0, 0, 0}, particleSpawnCenter_);
      emitterBoxModel_.SetWorldTransform(world);
      emitterBoxModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
    } else {
      Vector3 radius = particleSpawnExtent_;
      const float kMinRadius = 0.001f;
      radius.x = std::max(radius.x, kMinRadius);
      radius.y = std::max(radius.y, kMinRadius);
      radius.z = std::max(radius.z, kMinRadius);

      Vector3 scale = radius;
      Matrix4x4 world = MakeAffineMatrix(scale, {0, 0, 0}, particleSpawnCenter_);
      emitterSphereModel_.SetWorldTransform(world);
      emitterSphereModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
    }
  }

  {
    UnifiedPipeline *currentParticlePipeline = &particlePipelineAlpha_;
    switch (particleBlendMode_) {
    case 1: currentParticlePipeline = &particlePipelineAdd_; break;
    case 2: currentParticlePipeline = &particlePipelineSub_; break;
    case 3: currentParticlePipeline = &particlePipelineMul_; break;
    case 4: currentParticlePipeline = &particlePipelineScreen_; break;
    default: break;
    }

    cmdList->SetGraphicsRootSignature(currentParticlePipeline->GetRootSignature());
    cmdList->SetPipelineState(currentParticlePipeline->GetPipelineState());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &particleVBView_);
    cmdList->IASetIndexBuffer(&particleIBView_);

    ID3D12DescriptorHeap *heaps[] = {dx_.GetSRVHeap()};
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, particleMaterialCB_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, particleTextureHandle_);
    cmdList->SetGraphicsRootDescriptorTable(2, particleMatricesSrvGPU_);

    UINT indexCount = 6;
    UINT instanceCount = activeParticleCount_;
    cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
  }

#ifdef USE_IMGUI
  imgui_.Draw(cmdList);
#endif

  dx_.EndFrame();
}

void GameApp::InitLogging_() {
  std::filesystem::create_directory("logs");

  auto now = std::chrono::system_clock::now();
  auto nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
  std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};
  std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
  std::string logFilePath = std::string("logs/") + dateString + ".log";
  logStream_.open(logFilePath);
}

void GameApp::InitPipelines_() {
  auto *device = dx_.GetDevice();
  auto *dxcUtils = dx_.GetDXCUtils();
  auto *dxcCompiler = dx_.GetDXCCompiler();
  auto *includeHandler = dx_.GetDXCIncludeHandler();

  PipelineDesc objBase = UnifiedPipeline::MakeObject3DDesc();
  assert(objPipeline_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, objBase));

  {
    auto wire = objBase;
    wire.alphaBlend = false;
    wire.enableDepth = true;
    wire.cullMode = D3D12_CULL_MODE_NONE;
    wire.fillMode = D3D12_FILL_MODE_WIREFRAME;
    assert(emitterGizmoPipelineWire_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, wire));
  }

  PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

  auto sprAlpha = sprBase;
  sprAlpha.blendMode = BlendMode::Alpha;
  assert(spritePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprAlpha));

  auto sprAdd = sprBase;
  sprAdd.blendMode = BlendMode::Add;
  assert(spritePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprAdd));

  auto sprSub = sprBase;
  sprSub.blendMode = BlendMode::Subtract;
  assert(spritePipelineSub_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprSub));

  auto sprMul = sprBase;
  sprMul.blendMode = BlendMode::Multiply;
  assert(spritePipelineMul_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprMul));

  auto sprScr = sprBase;
  sprScr.blendMode = BlendMode::Screen;
  assert(spritePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprScr));

  PipelineDesc partBase = UnifiedPipeline::MakeParticleDesc();

  auto pAlpha = partBase;
  pAlpha.blendMode = BlendMode::Alpha;
  assert(particlePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pAlpha));

  auto pAdd = partBase;
  pAdd.blendMode = BlendMode::Add;
  assert(particlePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pAdd));

  auto pSub = partBase;
  pSub.blendMode = BlendMode::Subtract;
  assert(particlePipelineSub_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pSub));

  auto pMul = partBase;
  pMul.blendMode = BlendMode::Multiply;
  assert(particlePipelineMul_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pMul));

  auto pScr = partBase;
  pScr.blendMode = BlendMode::Screen;
  assert(particlePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pScr));
}

void GameApp::InitResources_() {
  ComPtr<ID3D12Device> device;
  dx_.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

  // ===== Model =====
  {
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &objPipeline_;
    ci.srvAlloc = &dx_.GetSrvAllocator();
    ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    bool okModel = model_.Initialize(ci);
    assert(okModel);
  }
  {
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &objPipeline_;
    ci.srvAlloc = &dx_.GetSrvAllocator();
    ci.modelData = LoadObjFile("resources/plane", "plane.obj");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    bool okPlane = planeModel_.Initialize(ci);
    assert(okPlane);
  }
  {
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.srvAlloc = &dx_.GetSrvAllocator();
    ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
    ci.baseColor = {0.3f, 0.8f, 1.0f, 0.3f};
    ci.lightingMode = 0;
    bool ok = emitterSphereModel_.Initialize(ci);
    assert(ok);
  }
  {
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.srvAlloc = &dx_.GetSrvAllocator();
    ci.modelData = LoadObjFile("resources/cube", "cube.obj");
    ci.baseColor = {1.0f, 0.8f, 0.2f, 0.3f};
    ci.lightingMode = 0;
    bool ok = emitterBoxModel_.Initialize(ci);
    assert(ok);
  }

  // ===== Sprite =====
  {
    Sprite::CreateInfo sprInfo{};
    sprInfo.dx = &dx_;
    sprInfo.pipeline = &spritePipelineAlpha_;
    sprInfo.srvAlloc = &dx_.GetSrvAllocator();
    sprInfo.texturePath = "resources/uvChecker.png";
    sprInfo.size = {640.0f, 360.0f};
    sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};
    bool okSprite = sprite_.Initialize(sprInfo);
    assert(okSprite);
  }

  // ===== Particle Instance Buffer (StructuredBuffer SRV) =====
  {
    const uint32_t instanceCount = kParticleCount_;
    particleInstanceBuffer_ = CreateBufferResource(device, sizeof(ParticleForGPU) * instanceCount);

    particleInstanceBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&particleMatrices_));
    for (uint32_t i = 0; i < instanceCount; ++i) {
      particleMatrices_[i].WVP = MakeIdentity4x4();
      particleMatrices_[i].World = MakeIdentity4x4();
      particleMatrices_[i].color = {1, 1, 1, 1};
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = instanceCount;
    desc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    UINT index = dx_.GetSrvAllocator().Allocate();
    device->CreateShaderResourceView(particleInstanceBuffer_.Get(), &desc, dx_.GetSrvAllocator().Cpu(index));
    particleMatricesSrvGPU_ = dx_.GetSrvAllocator().Gpu(index);
  }

  // 初期生成
  particles_.clear();
  uint32_t initialCount = std::min(initialParticleCount_, kParticleCount_);
  for (uint32_t i = 0; i < initialCount; ++i) {
    Particle p{};
    RespawnParticle_(p);
    particles_.push_back(p);
  }
  activeParticleCount_ = initialCount;

  // ===== Particle Material CB =====
  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(ParticleMaterialData);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&particleMaterialCB_));
    assert(SUCCEEDED(hr));

    particleMaterialCB_->Map(0, nullptr, reinterpret_cast<void **>(&particleMaterialData_));
    particleMaterialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    particleMaterialData_->enableLighting = 0;
    particleMaterialData_->pad[0] = particleMaterialData_->pad[1] = particleMaterialData_->pad[2] = 0.0f;
    particleMaterialData_->uvTransform = MakeIdentity4x4();
  }

  // ===== Particle Texture SRV (Texture2D) =====
  {
    DirectX::ScratchImage mipImages = LoadTexture("resources/particle/circle.png");
    const DirectX::TexMetadata &meta = mipImages.GetMetadata();

    particleTexture_ = CreateTextureResource(device, meta);
    UploadTextureData(particleTexture_, mipImages);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = meta.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);

    UINT texIndex = dx_.GetSrvAllocator().Allocate();

    // ここが重要：Texture2D のSRVには particleTexture_ を渡す
    device->CreateShaderResourceView(particleTexture_.Get(), &srvDesc, dx_.GetSrvAllocator().Cpu(texIndex));

    particleTextureHandle_ = dx_.GetSrvAllocator().Gpu(texIndex);
  }

  // ===== DirectionalLight CB =====
  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(DirectionalLight);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&directionalLightCB_));
    assert(SUCCEEDED(hr));

    directionalLightCB_->Map(0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
    *directionalLightData_ = {{1, 1, 1, 1}, {0, -1, 0}, 1.0f};
  }

  bool select = audio_.Load("select", L"resources/sound/select.mp3", 1.0f);
  assert(select);
  selectVol_ = 1.0f;

  struct Vtx { float px, py, pz; float u, v; };
  Vtx quad[4] = {
      {-0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
      { 0.5f,  0.5f, 0.0f, 1.0f, 0.0f},
      {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
      { 0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
  };
  uint16_t idx[6] = {0, 1, 2, 2, 1, 3};

  // VB
  {
    auto vbSize = sizeof(quad);
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = vbSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                IID_PPV_ARGS(&particleVB_));
    assert(SUCCEEDED(hr));

    void *mapped = nullptr;
    particleVB_->Map(0, nullptr, &mapped);
    memcpy(mapped, quad, vbSize);
    particleVB_->Unmap(0, nullptr);

    particleVBView_.BufferLocation = particleVB_->GetGPUVirtualAddress();
    particleVBView_.StrideInBytes = sizeof(Vtx);
    particleVBView_.SizeInBytes = static_cast<UINT>(vbSize);
  }

  // IB
  {
    auto ibSize = sizeof(idx);
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = ibSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                IID_PPV_ARGS(&particleIB_));
    assert(SUCCEEDED(hr));

    void *mapped = nullptr;
    particleIB_->Map(0, nullptr, &mapped);
    memcpy(mapped, idx, ibSize);
    particleIB_->Unmap(0, nullptr);

    particleIBView_.BufferLocation = particleIB_->GetGPUVirtualAddress();
    particleIBView_.Format = DXGI_FORMAT_R16_UINT;
    particleIBView_.SizeInBytes = static_cast<UINT>(ibSize);
  }
}

void GameApp::InitCamera_() {
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  cameraTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};
  transformSprite_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  uvTransformSprite_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  transform2_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}};
}

Vector4 GameApp::GenerateParticleColor_() {
  Vector3 rgb{1.0f, 1.0f, 1.0f};

  switch (particleColorMode_) {
  case ParticleColorMode::RandomRGB:
    rgb = {Rand01(), Rand01(), Rand01()};
    break;

  case ParticleColorMode::RangeRGB: {
    auto randSigned = []() { return Rand01() * 2.0f - 1.0f; };
    rgb.x = particleBaseColor_.x + randSigned() * particleColorRangeRGB_.x;
    rgb.y = particleBaseColor_.y + randSigned() * particleColorRangeRGB_.y;
    rgb.z = particleBaseColor_.z + randSigned() * particleColorRangeRGB_.z;
    rgb.x = std::clamp(rgb.x, 0.0f, 1.0f);
    rgb.y = std::clamp(rgb.y, 0.0f, 1.0f);
    rgb.z = std::clamp(rgb.z, 0.0f, 1.0f);
  } break;

  case ParticleColorMode::RangeHSV: {
    auto randSigned = []() { return Rand01() * 2.0f - 1.0f; };
    Vector3 hsv = particleBaseHSV_;
    hsv.x += randSigned() * particleHSVRange_.x;
    hsv.y += randSigned() * particleHSVRange_.y;
    hsv.z += randSigned() * particleHSVRange_.z;

    hsv.x = std::fmod(hsv.x, 1.0f);
    if (hsv.x < 0.0f) {
      hsv.x += 1.0f;
    }
    hsv.y = std::clamp(hsv.y, 0.0f, 1.0f);
    hsv.z = std::clamp(hsv.z, 0.0f, 1.0f);

    rgb = HSVtoRGB(hsv);
  } break;

  case ParticleColorMode::Fixed:
    rgb = {particleBaseColor_.x, particleBaseColor_.y, particleBaseColor_.z};
    break;
  }

  return {rgb.x, rgb.y, rgb.z, particleBaseColor_.w};
}

void GameApp::RespawnParticle_(Particle &p) {
  auto rndSigned = []() { return Rand01() * 2.0f - 1.0f; };

  Vector3 pos = particleSpawnCenter_;

  if (emitterShape_ == EmitterShape::Box) {
    Vector3 offset{
        rndSigned() * particleSpawnExtent_.x,
        rndSigned() * particleSpawnExtent_.y,
        rndSigned() * particleSpawnExtent_.z,
    };
    pos.x += offset.x;
    pos.y += offset.y;
    pos.z += offset.z;
  } else {
    Vector3 radius = particleSpawnExtent_;
    const float kMinRadius = 0.001f;
    radius.x = std::max(radius.x, kMinRadius);
    radius.y = std::max(radius.y, kMinRadius);
    radius.z = std::max(radius.z, kMinRadius);

    Vector3 local{};
    while (true) {
      local.x = RandRange(-1.0f, 1.0f);
      local.y = RandRange(-1.0f, 1.0f);
      local.z = RandRange(-1.0f, 1.0f);
      float len2 = local.x * local.x + local.y * local.y + local.z * local.z;
      if (len2 <= 1.0f) {
        break;
      }
    }
    pos.x += local.x * radius.x;
    pos.y += local.y * radius.y;
    pos.z += local.z * radius.z;
  }

  p.transform.translate = pos;
  p.transform.scale = {0.5f, 0.5f, 0.5f};
  p.transform.rotate = {0.0f, 0.0f, 0.0f};

  Vector3 dir = particleBaseDir_;
  dir.x += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  dir.y += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  dir.z += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  if (Length(dir) > 0.0001f) {
    dir = Normalize(dir);
  } else {
    dir = {0.0f, 1.0f, 0.0f};
  }

  float speed = RandRange(particleSpeedMin_, particleSpeedMax_);
  p.velocity = {dir.x * speed, dir.y * speed, dir.z * speed};

  p.lifetime = RandRange(particleLifeMin_, particleLifeMax_);
  p.age = 0.0f;

  p.color = GenerateParticleColor_();
}

Matrix4x4 GameApp::MakeBillboardMatrix(const Vector3 &scale, const Vector3 &translate,
                                       const Matrix4x4 &viewMatrix) {
  Matrix4x4 camWorld = Inverse(viewMatrix);
  Vector3 right{camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2]};
  Vector3 up{camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2]};
  Vector3 forward{camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2]};
  forward = {-forward.x, -forward.y, -forward.z};

  Matrix4x4 m{};
  m.m[0][0] = scale.x * right.x;
  m.m[0][1] = scale.x * right.y;
  m.m[0][2] = scale.x * right.z;
  m.m[0][3] = 0;

  m.m[1][0] = scale.y * up.x;
  m.m[1][1] = scale.y * up.y;
  m.m[1][2] = scale.y * up.z;
  m.m[1][3] = 0;

  m.m[2][0] = scale.z * forward.x;
  m.m[2][1] = scale.z * forward.y;
  m.m[2][2] = scale.z * forward.z;
  m.m[2][3] = 0;

  m.m[3][0] = translate.x;
  m.m[3][1] = translate.y;
  m.m[3][2] = translate.z;
  m.m[3][3] = 1;

  return m;
}

bool GameApp::IsCollision(const AABB &aabb, const Vector3 &point) {
  if (point.x < aabb.min.x || point.x > aabb.max.x) return false;
  if (point.y < aabb.min.y || point.y > aabb.max.y) return false;
  if (point.z < aabb.min.z || point.z > aabb.max.z) return false;
  return true;
}
