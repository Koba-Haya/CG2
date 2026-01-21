#define NOMINMAX
#include "GameApp.h"

#include "DebugCamera.h"
#include "GameCamera.h"
#include "AssetLoader.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <random>
#include <string>

static void FatalBoxAndTerminate_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Fatal", MB_OK | MB_ICONERROR);
  std::terminate();
}

static void CheckBoolOrDie_(bool ok, const char *what) {
  if (!ok) {
    FatalBoxAndTerminate_(std::string("[GameApp] failed: ") + what);
  }
}

static void CheckHROrDie_(HRESULT hr, const char *what) {
  if (FAILED(hr)) {
    char buf[512];
    sprintf_s(buf, "[GameApp] HRESULT failed: %s (hr=0x%08X)", what,
              (unsigned)hr);
    FatalBoxAndTerminate_(buf);
  }
}

static void CheckFileExists_(const std::string &path) {
  if (!std::filesystem::exists(path)) {
    FatalBoxAndTerminate_(std::string("File not found:\n") + path);
  }
}

namespace {
std::mt19937 &GetRNG() {
  static std::mt19937 rng{std::random_device{}()};
  return rng;
}
float Rand01() {
  static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  return dist(GetRNG());
}
float RandRange(float minV, float maxV) {
  return minV + (maxV - minV) * Rand01();
}

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
  case 0:
    return {v, t, p};
  case 1:
    return {q, v, p};
  case 2:
    return {p, v, t};
  case 3:
    return {p, q, v};
  case 4:
    return {t, p, v};
  default:
    return {v, p, q};
  }
}
} // namespace

static Matrix4x4 MakeUVMatrixFromTransform(const Transform &uvT) {
  Vector3 scale = {uvT.scale.x, uvT.scale.y, 1.0f};
  float rotZ = uvT.rotate.z;
  Vector3 trans = {uvT.translate.x, uvT.translate.y, 0.0f};

  Matrix4x4 T0 = MakeTranslateMatrix({-0.5f, -0.5f, 0.0f});
  Matrix4x4 S = MakeScaleMatrix(scale);
  Matrix4x4 R = MakeRotateZMatrix(rotZ);
  Matrix4x4 T1 = MakeTranslateMatrix({0.5f, 0.5f, 0.0f});
  Matrix4x4 T = MakeTranslateMatrix(trans);

  Matrix4x4 m = Multiply(T0, Multiply(S, Multiply(R, Multiply(T1, T))));
  return m;
}

GameApp::GameApp() = default;
GameApp::~GameApp() = default;

void GameApp::Initialize() {
  InitLogging_();

  auto &dx = GetDX();
  shaderCompiler_.Initialize(dx.GetDXCUtils(), dx.GetDXCCompiler(),
                             dx.GetDXCIncludeHandler());

  InitPipelines_();
  InitResources_();

  camera_ = std::make_unique<DebugCamera>();
  camera_->Initialize();
  InitCamera_();

  accelerationField_.acceleration = {15.0f, 0.0f, 0.0f};
  accelerationField_.area.min = {-1.0f, -1.0f, -1.0f};
  accelerationField_.area.max = {1.0f, 1.0f, 1.0f};
}

void GameApp::Finalize() {
  // ManagerのFinalizeはFramework側でやる
  // Game固有の後片付けがあるならここに書く（ファイル、ログなど）
  if (logStream_.is_open()) {
    logStream_.flush();
    logStream_.close();
  }
}

void GameApp::Update() {
  auto &input = GetInput();

  if (camera_) {
    camera_->Update(input);
    view3D_ = camera_->GetViewMatrix();
    proj3D_ = camera_->GetProjectionMatrix();
  } else {
    view3D_ = Inverse(MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
                                       {0.0f, 0.0f, -10.0f}));
    proj3D_ = MakePerspectiveFovMatrix(
        0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
        100.0f);
  }

  if (cameraData_) {
    Matrix4x4 invView = Inverse(view3D_);
    cameraData_->worldPosition = {invView.m[3][0], invView.m[3][1],
                                  invView.m[3][2]};
    cameraData_->pad = 0.0f;
  }

#ifdef USE_IMGUI
  GetImGui().Begin();

  static bool settingsOpen = true;
  ImGui::Begin("Settings", &settingsOpen);

  ImGui::Text("Light");

  if (directionalLightData_) {
    ImGui::Checkbox("Enable DirectionalLight", &enableDirectionalLight_);
    directionalLightData_->enabled = enableDirectionalLight_ ? 1 : 0;

    if (enableDirectionalLight_) {
      static float lightDir[3] = {0.0f, -1.0f, 0.0f};
      static bool lightDirInit = false;
      if (!lightDirInit) {
        lightDir[0] = directionalLightData_->direction.x;
        lightDir[1] = directionalLightData_->direction.y;
        lightDir[2] = directionalLightData_->direction.z;
        lightDirInit = true;
      }

      if (ImGui::SliderFloat3("LightDirection", lightDir, -1.0f, 1.0f)) {
        Vector3 dir = {lightDir[0], lightDir[1], lightDir[2]};
        directionalLightData_->direction = Normalize(dir);
      }

      ImGui::ColorEdit3("LightColor", reinterpret_cast<float *>(
                                          &directionalLightData_->color));
    }
  }

  if (pointLightData_) {
    ImGui::Separator();
    ImGui::Text("PointLight");

    ImGui::Checkbox("Enable PointLight", &enablePointLight_);
    pointLightData_->enabled = enablePointLight_ ? 1 : 0;

    if (enablePointLight_) {
      ImGui::ColorEdit3("PointColor",
                        reinterpret_cast<float *>(&pointLightData_->color));
      ImGui::DragFloat3("PointPosition",
                        reinterpret_cast<float *>(&pointLightData_->position),
                        0.01f);
      ImGui::SliderFloat("PointRadius", &pointLightData_->radius, 0.01f, 50.0f);
      ImGui::SliderFloat("PointDecay", &pointLightData_->decay, 0.01f, 8.0f);
    }
  }

  if (spotLightData_) {
    ImGui::Separator();
    ImGui::Text("SpotLight");

    ImGui::Checkbox("Enable SpotLight", &enableSpotLight_);
    spotLightData_->enabled = enableSpotLight_ ? 1 : 0;

    if (enableSpotLight_) {
      ImGui::ColorEdit3("SpotColor",
                        reinterpret_cast<float *>(&spotLightData_->color));
      ImGui::DragFloat3("SpotPosition",
                        reinterpret_cast<float *>(&spotLightData_->position),
                        0.01f);

      static float dir[3] = {0.0f, -1.0f, 0.0f};
      if (ImGui::SliderFloat3("SpotDirection", dir, -1.0f, 1.0f)) {
        spotLightData_->direction = Normalize(Vector3{dir[0], dir[1], dir[2]});
      }

      ImGui::SliderFloat("SpotDistance", &spotLightData_->distance, 0.01f, 50.0f);
      ImGui::SliderFloat("SpotDecay", &spotLightData_->decay, 0.01f, 8.0f);

      static float coneAngleDeg = 30.0f;
      if (ImGui::SliderFloat("Cone Angle (deg)", &coneAngleDeg, 1.0f, 89.0f)) {
        float rad = coneAngleDeg * (3.14159265f / 180.0f);
        spotLightData_->cosAngle = std::cos(rad);
      }
    }
  }

  static float sphereCol[3] = {1.0f, 1.0f, 1.0f};
  ImGui::Text("ObjectColor");
  if (ImGui::ColorEdit3("SphereColor", sphereCol)) {
    model_.SetColor({sphereCol[0], sphereCol[1], sphereCol[2], 1.0f});
  }

  static float planeCol[3] = {1.0f, 1.0f, 1.0f};
  if (ImGui::ColorEdit3("PlaneColor", planeCol)) {
    planeModel_.SetColor({planeCol[0], planeCol[1], planeCol[2], 1.0f});
  }

  static float spriteCol[3] = {1.0f, 1.0f, 1.0f};
  if (ImGui::ColorEdit3("SpriteColor", spriteCol)) {
    sprite_.SetColor({spriteCol[0], spriteCol[1], spriteCol[2], 1.0f});
  }

  ImGui::Text("Lighting Mode");
  ImGui::RadioButton("None", &lightingMode_, 0);
  ImGui::RadioButton("Lambert", &lightingMode_, 1);
  ImGui::RadioButton("Half-Lambert", &lightingMode_, 2);

  const char *blendModeItems[] = {
      "Alpha (通常)",    "Add (加算)",          "Subtract (減算)",
      "Multiply (乗算)", "Screen (スクリーン)",
  };
  ImGui::Combo("Sprite Blend", &spriteBlendMode_, blendModeItems,
               IM_ARRAYSIZE(blendModeItems));

  static bool useTransformCamera = false;
  ImGui::Separator();
  ImGui::Text("Camera");
  ImGui::Checkbox("Use Transform Camera", &useTransformCamera);

  ImGui::DragFloat3("CameraTranslate",
                    reinterpret_cast<float *>(&cameraTransform_.translate),
                    0.01f);
  ImGui::DragFloat3("CameraRotate",
                    reinterpret_cast<float *>(&cameraTransform_.rotate), 0.01f);

  ImGui::Separator();
  ImGui::Text("Sphere");
  ImGui::DragFloat3("SphereTranslate",
                    reinterpret_cast<float *>(&transform_.translate), 0.01f);
  ImGui::DragFloat3("SphereRotate",
                    reinterpret_cast<float *>(&transform_.rotate), 0.01f);
  ImGui::DragFloat3("SphereScale", reinterpret_cast<float *>(&transform_.scale),
                    0.01f, 0.0f, 5.0f);

  ImGui::Separator();
  ImGui::Text("Plane");
  ImGui::DragFloat3("PlaneTranslate", &transform2_.translate.x, 0.01f);
  ImGui::DragFloat3("PlaneRotate", &transform2_.rotate.x, 0.01f);
  ImGui::DragFloat3("PlaneScale", &transform2_.scale.x, 0.01f, 0.0f, 5.0f);

  ImGui::Separator();
  ImGui::Text("Sprite");
  ImGui::DragFloat3("SpriteTranslate", &transformSprite_.translate.x, 1.0f);
  ImGui::DragFloat3("SpriteRotate", &transformSprite_.rotate.x, 0.01f);
  ImGui::DragFloat3("SpriteScale", &transformSprite_.scale.x, 0.01f);

  ImGui::Separator();
  ImGui::Text("UV");
  ImGui::DragFloat2("UVTranslate", &uvTransformSprite_.translate.x, 0.01f,
                    -10.0f, 10.0f);
  ImGui::DragFloat2("UVScale", &uvTransformSprite_.scale.x, 0.01f, -10.0f,
                    10.0f);
  ImGui::SliderAngle("UVRotate", &uvTransformSprite_.rotate.z);

  ImGui::End();

  {
    if (useTransformCamera) {
      Matrix4x4 camWorld =
          MakeAffineMatrix({1.0f, 1.0f, 1.0f}, cameraTransform_.rotate,
                           cameraTransform_.translate);
      view3D_ = Inverse(camWorld);

      if (cameraData_) {
        cameraData_->worldPosition = cameraTransform_.translate;
        cameraData_->pad = 0.0f;
      }
    }
  }

  ImGui::Begin("Particles");

  auto &ep = particleEmitter_.GetParams();

  ImGui::SeparatorText("Emitter");

  ImGui::Checkbox("Show Emitter Gizmo", &showEmitterGizmo_);

  {
    int shapeIndex = (ep.shape == EmitterShape::Box) ? 0 : 1;
    const char *shapeItems[] = {"Box", "Sphere"};
    if (ImGui::Combo("Emitter Shape", &shapeIndex, shapeItems, 2)) {
      ep.shape = (shapeIndex == 0) ? EmitterShape::Box : EmitterShape::Sphere;
    }
  }

  Vector3 emitterMin = {ep.localCenter.x - ep.extent.x,
                        ep.localCenter.y - ep.extent.y,
                        ep.localCenter.z - ep.extent.z};
  Vector3 emitterMax = {ep.localCenter.x + ep.extent.x,
                        ep.localCenter.y + ep.extent.y,
                        ep.localCenter.z + ep.extent.z};

  ImGui::DragFloat3("Emitter Min", reinterpret_cast<float *>(&emitterMin),
                    0.01f);
  ImGui::DragFloat3("Emitter Max", reinterpret_cast<float *>(&emitterMax),
                    0.01f);

  emitterMin.x = std::min(emitterMin.x, emitterMax.x);
  emitterMin.y = std::min(emitterMin.y, emitterMax.y);
  emitterMin.z = std::min(emitterMin.z, emitterMax.z);

  emitterMax.x = std::max(emitterMin.x, emitterMax.x);
  emitterMax.y = std::max(emitterMin.y, emitterMax.y);
  emitterMax.z = std::max(emitterMin.z, emitterMax.z);

  ep.localCenter = {(emitterMin.x + emitterMax.x) * 0.5f,
                    (emitterMin.y + emitterMax.y) * 0.5f,
                    (emitterMin.z + emitterMax.z) * 0.5f};
  ep.extent = {(emitterMax.x - emitterMin.x) * 0.5f,
               (emitterMax.y - emitterMin.y) * 0.5f,
               (emitterMax.z - emitterMin.z) * 0.5f};

  ImGui::SliderFloat("Emit Rate (per sec)", &ep.emitRate, 0.0f, 500.0f);

  ImGui::SliderFloat("Life Min", &ep.lifeMin, 0.01f, 20.0f);
  ImGui::SliderFloat("Life Max", &ep.lifeMax, 0.01f, 20.0f);
  if (ep.lifeMax < ep.lifeMin) {
    ep.lifeMax = ep.lifeMin;
  }

  {
    int mode = static_cast<int>(ep.colorMode);
    const char *items[] = {"RandomRGB", "RangeRGB", "RangeHSV", "Fixed"};
    ImGui::Combo("Color Mode", &mode, items, 4);
    ep.colorMode = static_cast<ParticleColorMode>(mode);
  }

  ImGui::ColorEdit4("Base Color", reinterpret_cast<float *>(&ep.baseColor));

  if (ep.colorMode == ParticleColorMode::RangeRGB) {
    ImGui::SliderFloat3("RGB Range (+/-)",
                        reinterpret_cast<float *>(&ep.rgbRange), 0.0f, 1.0f);
  }

  if (ep.colorMode == ParticleColorMode::RangeHSV) {
    ImGui::SliderFloat3("Base HSV", reinterpret_cast<float *>(&ep.baseHSV),
                        0.0f, 1.0f);
    ImGui::SliderFloat3("HSV Range (+/-)",
                        reinterpret_cast<float *>(&ep.hsvRange), 0.0f, 1.0f);
  }

  if (ImGui::Button("Reset Particles")) {
    ParticleManager::GetInstance()->ClearParticleGroup(ep.groupName);
  }

  ImGui::SeparatorText("Manager");

  static int maxInstances = 1000;
  ImGui::SliderInt("Max Particles", &maxInstances, 0,
                   static_cast<int>(kParticleCount_));
  maxInstances = std::clamp(maxInstances, 0, static_cast<int>(kParticleCount_));
  ParticleManager::GetInstance()->SetGroupInstanceLimit(
      ep.groupName, static_cast<uint32_t>(maxInstances));

  {
    const char *blendItems[] = {"Alpha", "Add", "Subtract", "Multiply",
                                "Screen"};
    ImGui::Combo("Blend Mode", &particleBlendMode_, blendItems, 5);
    particleBlendMode_ = std::clamp(particleBlendMode_, 0, 4);
  }

  ImGui::Checkbox("Enable Acceleration Field", &enableAccelerationField_);
  ImGui::DragFloat3("Accel",
                    reinterpret_cast<float *>(&accelerationField_.acceleration),
                    0.1f, -100.0f, 100.0f);

  ImGui::DragFloat3("Field AABB Min",
                    reinterpret_cast<float *>(&accelerationField_.area.min),
                    0.1f);
  ImGui::DragFloat3("Field AABB Max",
                    reinterpret_cast<float *>(&accelerationField_.area.max),
                    0.1f);

  accelerationField_.area.min.x =
      std::min(accelerationField_.area.min.x, accelerationField_.area.max.x);
  accelerationField_.area.min.y =
      std::min(accelerationField_.area.min.y, accelerationField_.area.max.y);
  accelerationField_.area.min.z =
      std::min(accelerationField_.area.min.z, accelerationField_.area.max.z);

  accelerationField_.area.max.x =
      std::max(accelerationField_.area.min.x, accelerationField_.area.max.x);
  accelerationField_.area.max.y =
      std::max(accelerationField_.area.min.y, accelerationField_.area.max.y);
  accelerationField_.area.max.z =
      std::max(accelerationField_.area.min.z, accelerationField_.area.max.z);

  ImGui::End();

  GetImGui().End();
#endif

  const float deltaTime = 1.0f / 60.0f;

  particleEmitter_.Update(deltaTime);

  ParticleManager::GetInstance()->SetEnableAccelerationField(
      enableAccelerationField_);
  ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);
  ParticleManager::GetInstance()->Update(view3D_, proj3D_, deltaTime);
}

void GameApp::Draw() {
  auto &dx = GetDX();

  dx.BeginFrame();
  auto *cmdList = dx.GetCommandList();

  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      dx.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

  UINT backBufferIndex = dx.GetSwapChain()->GetCurrentBackBufferIndex();
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx.GetRTVHandle(backBufferIndex);

  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                 nullptr);

  cmdList->RSSetViewports(1, &dx.GetViewport());
  cmdList->RSSetScissorRects(1, &dx.GetScissorRect());

  Matrix4x4 viewMatrix = view3D_;
  Matrix4x4 projectionMatrix = proj3D_;

  {
    Matrix4x4 worldSphere = MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate);
    model_.SetWorldTransform(worldSphere);
    model_.SetLightingMode(lightingMode_);

    Matrix4x4 worldPlane = MakeAffineMatrix(
        transform2_.scale, transform2_.rotate, transform2_.translate);
    planeModel_.SetWorldTransform(worldPlane);

    terrainTransform_.translate = {0.0f, -3.0f, 0.0f};
    Matrix4x4 worldTerrain = MakeAffineMatrix(terrainTransform_.scale, terrainTransform_.rotate,
                         terrainTransform_.translate);
    terrainModel_.SetWorldTransform(worldTerrain);

    cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
    cmdList->SetPipelineState(objPipeline_.GetPipelineState());

    model_.SetSpecularColor({1.0f, 1.0f, 1.0f});
    model_.SetShininess(64.0f);

    model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get(),
                cameraCB_.Get(), pointLightCB_.Get(), spotLightCB_.Get());
    planeModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get(),
                     cameraCB_.Get(), pointLightCB_.Get(), spotLightCB_.Get());
    terrainModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get(),
                       cameraCB_.Get(), pointLightCB_.Get(),
                       spotLightCB_.Get());
  }

  {
    UnifiedPipeline *currentParticlePipeline = &particlePipelineAlpha_;
    switch (particleBlendMode_) {
    case 1:
      currentParticlePipeline = &particlePipelineAdd_;
      break;
    case 2:
      currentParticlePipeline = &particlePipelineSub_;
      break;
    case 3:
      currentParticlePipeline = &particlePipelineMul_;
      break;
    case 4:
      currentParticlePipeline = &particlePipelineScreen_;
      break;
    default:
      break;
    }
    //ParticleManager::GetInstance()->Draw(cmdList, currentParticlePipeline);
  }

  {
    UnifiedPipeline *currentSpritePipeline = &spritePipelineAlpha_;
    switch (spriteBlendMode_) {
    case 1:
      currentSpritePipeline = &spritePipelineAdd_;
      break;
    case 2:
      currentSpritePipeline = &spritePipelineSub_;
      break;
    case 3:
      currentSpritePipeline = &spritePipelineMul_;
      break;
    case 4:
      currentSpritePipeline = &spritePipelineScreen_;
      break;
    default:
      break;
    }

    sprite_.SetPipeline(currentSpritePipeline);

    sprite_.SetPosition(transformSprite_.translate);
    sprite_.SetRotation(transformSprite_.rotate);
    sprite_.SetScale(transformSprite_.scale);

    Matrix4x4 uvMat = MakeUVMatrixFromTransform(uvTransformSprite_);
    sprite_.SetUVTransform(uvMat);

    const auto &vp = dx.GetViewport();
    const float screenW = vp.Width;
    const float screenH = vp.Height;

    Matrix4x4 view2D = MakeIdentity4x4();
    Matrix4x4 proj2D =
        MakeOrthographicMatrix(0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f);

    //sprite_.Draw(view2D, proj2D);
  }

  {
    if (showEmitterGizmo_) {
      const auto &ep = particleEmitter_.GetParams();

      if (ep.shape == EmitterShape::Box) {
        Vector3 scale = {ep.extent.x * 2.0f, ep.extent.y * 2.0f,
                         ep.extent.z * 2.0f};
        Matrix4x4 world =
            MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
        emitterBoxModel_.SetWorldTransform(world);
        emitterBoxModel_.Draw(viewMatrix, projectionMatrix,
                              directionalLightCB_.Get(), cameraCB_.Get(),
                              pointLightCB_.Get(), spotLightCB_.Get());
      } else {
        Vector3 scale = {std::max(ep.extent.x, 0.001f),
                         std::max(ep.extent.y, 0.001f),
                         std::max(ep.extent.z, 0.001f)};
        Matrix4x4 world =
            MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
        emitterSphereModel_.SetWorldTransform(world);
        emitterSphereModel_.Draw(viewMatrix, projectionMatrix,
                                 directionalLightCB_.Get(), cameraCB_.Get(),
                                 pointLightCB_.Get(), spotLightCB_.Get());
      }
    }
  }

#ifdef USE_IMGUI
  GetImGui().Draw(cmdList);
#endif

  dx.EndFrame();
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
  auto &dx = GetDX();

  auto *device = dx.GetDevice();
  auto *dxcUtils = dx.GetDXCUtils();
  auto *dxcCompiler = dx.GetDXCCompiler();
  auto *includeHandler = dx.GetDXCIncludeHandler();

  PipelineDesc objBase = UnifiedPipeline::MakeObject3DDesc();
  CheckBoolOrDie_(objPipeline_.Initialize(device, dxcUtils, dxcCompiler,
                                          includeHandler, objBase),
                  "objPipeline_.Initialize");

  {
    auto wire = objBase;
    wire.alphaBlend = false;
    wire.enableDepth = true;
    wire.cullMode = D3D12_CULL_MODE_NONE;
    wire.fillMode = D3D12_FILL_MODE_WIREFRAME;

    CheckBoolOrDie_(emitterGizmoPipelineWire_.Initialize(
                        device, dxcUtils, dxcCompiler, includeHandler, wire),
                    "emitterGizmoPipelineWire_.Initialize");
  }

  PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

  auto sprAlpha = sprBase;
  sprAlpha.blendMode = BlendMode::Alpha;
  CheckBoolOrDie_(spritePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler,
                                                  includeHandler, sprAlpha),
                  "spritePipelineAlpha_.Initialize");

  auto sprAdd = sprBase;
  sprAdd.blendMode = BlendMode::Add;
  CheckBoolOrDie_(spritePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
                                                includeHandler, sprAdd),
                  "spritePipelineAdd_.Initialize");

  auto sprSub = sprBase;
  sprSub.blendMode = BlendMode::Subtract;
  CheckBoolOrDie_(spritePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
                                                includeHandler, sprSub),
                  "spritePipelineSub_.Initialize");

  auto sprMul = sprBase;
  sprMul.blendMode = BlendMode::Multiply;
  CheckBoolOrDie_(spritePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
                                                includeHandler, sprMul),
                  "spritePipelineMul_.Initialize");

  auto sprScr = sprBase;
  sprScr.blendMode = BlendMode::Screen;
  CheckBoolOrDie_(spritePipelineScreen_.Initialize(
                      device, dxcUtils, dxcCompiler, includeHandler, sprScr),
                  "spritePipelineScreen_.Initialize");

  PipelineDesc partBase = UnifiedPipeline::MakeParticleDesc();

  auto pAlpha = partBase;
  pAlpha.blendMode = BlendMode::Alpha;
  CheckBoolOrDie_(particlePipelineAlpha_.Initialize(
                      device, dxcUtils, dxcCompiler, includeHandler, pAlpha),
                  "particlePipelineAlpha_.Initialize");

  auto pAdd = partBase;
  pAdd.blendMode = BlendMode::Add;
  CheckBoolOrDie_(particlePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
                                                  includeHandler, pAdd),
                  "particlePipelineAdd_.Initialize");

  auto pSub = partBase;
  pSub.blendMode = BlendMode::Subtract;
  CheckBoolOrDie_(particlePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
                                                  includeHandler, pSub),
                  "particlePipelineSub_.Initialize");

  auto pMul = partBase;
  pMul.blendMode = BlendMode::Multiply;
  CheckBoolOrDie_(particlePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
                                                  includeHandler, pMul),
                  "particlePipelineMul_.Initialize");

  auto pScr = partBase;
  pScr.blendMode = BlendMode::Screen;
  CheckBoolOrDie_(particlePipelineScreen_.Initialize(
                      device, dxcUtils, dxcCompiler, includeHandler, pScr),
                  "particlePipelineScreen_.Initialize");
}

void GameApp::InitResources_() {
  auto &dx = GetDX();

  ComPtr<ID3D12Device> device;
  dx.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

  CheckFileExists_("resources/sphere/sphere.obj");
  CheckFileExists_("resources/plane/plane.gltf");
  CheckFileExists_("resources/cube/cube.obj");
  CheckFileExists_("resources/uvChecker.png");
  CheckFileExists_("resources/particle/circle.png");
  CheckFileExists_("resources/sound/select.mp3");

  auto *loader = AssetLoader::GetInstance();

  // 球モデル
  {
    Model::CreateInfo ci{};
    ci.dx = &dx;
    ci.pipeline = &objPipeline_;
    ci.modelData = loader->LoadModel("resources/sphere", "sphere.obj");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;

    const bool okModel = model_.Initialize(ci);
    CheckBoolOrDie_(okModel, "model_.Initialize(sphere)");
  }

  // 平面モデル
  {
    Model::CreateInfo ci{};
    ci.dx = &dx;
    ci.pipeline = &objPipeline_;
    ci.modelData = loader->LoadModel("resources/plane", "plane.gltf");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;

    const bool okPlane = planeModel_.Initialize(ci);
    CheckBoolOrDie_(okPlane, "planeModel_.Initialize(plane)");
  }

  // エミッタギズモモデル(球)
  {
    Model::CreateInfo ci{};
    ci.dx = &dx;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.modelData = loader->LoadModel("resources/sphere", "sphere.obj");
    ci.baseColor = {0.3f, 0.8f, 1.0f, 0.3f};
    ci.lightingMode = 0;

    const bool ok = emitterSphereModel_.Initialize(ci);
    CheckBoolOrDie_(ok, "emitterSphereModel_.Initialize");
  }

  // エミッタギズモモデル(箱)
  {
    Model::CreateInfo ci{};
    ci.dx = &dx;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.modelData = loader->LoadModel("resources/cube", "cube.obj");
    ci.baseColor = {1.0f, 0.8f, 0.2f, 0.3f};
    ci.lightingMode = 0;

    const bool ok = emitterBoxModel_.Initialize(ci);
    CheckBoolOrDie_(ok, "emitterBoxModel_.Initialize");
  }

  // terrainモデル
  {
    Model::CreateInfo ci{};
    ci.dx = &dx;
    ci.pipeline = &objPipeline_;
    ci.modelData = loader->LoadModel("resources/terrain", "terrain.obj");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    const bool ok = terrainModel_.Initialize(ci);
    CheckBoolOrDie_(ok, "terrainModel_.Initialize");
  }

  // スプライト
  {
    Sprite::CreateInfo sprInfo{};
    sprInfo.dx = &dx;
    sprInfo.pipeline = &spritePipelineAlpha_;
    sprInfo.texturePath = "resources/uvChecker.png";
    sprInfo.size = {640.0f, 360.0f};
    sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};

    const bool okSprite = sprite_.Initialize(sprInfo);
    CheckBoolOrDie_(okSprite, "sprite_.Initialize");
  }

  // パーティクル
  {
    const bool ok = ParticleManager::GetInstance()->CreateParticleGroup(
        particleGroupName_, "resources/particle/circle.png", kParticleCount_);
    CheckBoolOrDie_(ok, "ParticleManager::CreateParticleGroup");

    ParticleEmitter::Params params{};
    params.groupName = particleGroupName_;
    params.shape = EmitterShape::Box;
    params.localCenter = {0.0f, 0.0f, 0.0f};
    params.extent = {1.0f, 1.0f, 1.0f};
    params.baseDir = {0.0f, 1.0f, 0.0f};
    params.dirRandomness = 0.5f;
    params.speedMin = 0.5f;
    params.speedMax = 2.0f;
    params.lifeMin = 1.0f;
    params.lifeMax = 3.0f;
    params.particleScale = {0.5f, 0.5f, 0.5f};
    params.emitRate = 10.0f;
    params.colorMode = ParticleColorMode::RandomRGB;
    params.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

    particleEmitter_.Initialize(ParticleManager::GetInstance(), params);
    particleEmitter_.Burst(std::min(initialParticleCount_, kParticleCount_));
  }

  // Directional Light CB
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
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&directionalLightCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(directionalLightCB_)");

    hr = directionalLightCB_->Map(
        0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
    CheckHROrDie_(hr, "directionalLightCB_->Map");

    if (!directionalLightData_) {
      FatalBoxAndTerminate_("directionalLightData_ is null after Map");
    }

    directionalLightData_->color = {1, 1, 1, 1};
    directionalLightData_->direction = {0, -1, 0};
    directionalLightData_->intensity = 1.0f;
    directionalLightData_->enabled = 0;
  }

  // Camera CB
  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(CameraForGPU);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    auto *dev = dx.GetDevice();
    HRESULT hr = dev->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cameraCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(cameraCB_)");

    hr = cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));
    CheckHROrDie_(hr, "cameraCB_->Map");

    if (!cameraData_) {
      FatalBoxAndTerminate_("cameraData_ is null after Map");
    }

    cameraData_->worldPosition = {0.0f, 0.0f, -10.0f};
    cameraData_->pad = 0.0f;
  }

  // Point Light CB
  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(PointLight);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    auto *dev = dx.GetDevice();
    HRESULT hr =
        dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                     IID_PPV_ARGS(&pointLightCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(pointLightCB_)");

    hr = pointLightCB_->Map(0, nullptr,
                            reinterpret_cast<void **>(&pointLightData_));
    CheckHROrDie_(hr, "pointLightCB_->Map");

    if (!pointLightData_) {
      FatalBoxAndTerminate_("pointLightData_ is null after Map");
    }

    pointLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    pointLightData_->position = {0.0f, 2.0f, -2.0f};
    pointLightData_->intensity = 1.0f;
    pointLightData_->radius = 10.0f;
    pointLightData_->decay = 2.0f;
    pointLightData_->padding[0] = 0.0f;
    pointLightData_->padding[1] = 0.0f;
    pointLightData_->enabled = 1;
  }

  // Spot Light CB
  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(SpotLight);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    auto *dev = dx.GetDevice();
    HRESULT hr =
        dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                     IID_PPV_ARGS(&spotLightCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(spotLightCB_)");

    hr = spotLightCB_->Map(0, nullptr,
                           reinterpret_cast<void **>(&spotLightData_));
    CheckHROrDie_(hr, "spotLightCB_->Map");

    if (!spotLightData_) {
      FatalBoxAndTerminate_("spotLightData_ is null after Map");
    }

    spotLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    spotLightData_->position = {0.0f, 3.0f, -2.0f};
    spotLightData_->intensity = 1.0f;

    spotLightData_->direction = Normalize(Vector3{0.0f, -1.0f, 0.0f});
    spotLightData_->distance = 10.0f;
    spotLightData_->decay = 2.0f;

    float angleRad = 30.0f * (3.14159265f / 180.0f);
    spotLightData_->cosAngle = std::cos(angleRad);

    spotLightData_->enabled = 1;
    spotLightData_->padding[0] = 0.0f;
    spotLightData_->padding[1] = 0.0f;

    enableSpotLight_ = true;
  }

  {
    const bool select =
        GetAudio().Load("select", L"resources/sound/select.mp3", 1.0f);
    CheckBoolOrDie_(select, "audio_.Load(select.mp3)");
  }

  struct Vtx {
    float px, py, pz;
    float u, v;
  };
  Vtx quad[4] = {
      {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f},
      {0.5f, 0.5f, 0.0f, 1.0f, 0.0f},
      {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
      {0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
  };
  uint16_t idx[6] = {0, 1, 2, 2, 1, 3};
  (void)quad;
  (void)idx;
}

void GameApp::InitCamera_() {
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};
  transformSprite_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  transform2_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}};
  terrainTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  if (camera_) {
    camera_->SetPerspective(
        0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
        100.0f);
  }
}
