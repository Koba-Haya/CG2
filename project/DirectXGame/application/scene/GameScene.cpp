#define NOMINMAX
#include "GameScene.h"

#include "DebugCamera.h"
#include "GameCamera.h"
#include "TextureManager.h"
#include "TextureResource.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <random>

static constexpr UINT Align256_(UINT n) { return (n + 255u) & ~255u; }

static Microsoft::WRL::ComPtr<ID3D12Resource>
CreateUploadBuffer_(ID3D12Device *device, size_t size) {
  Microsoft::WRL::ComPtr<ID3D12Resource> res;

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = static_cast<UINT64>(size);
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  HRESULT hr = device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
  assert(SUCCEEDED(hr));
  return res;
}

static void FatalBoxAndTerminate_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Fatal", MB_OK | MB_ICONERROR);
  std::terminate();
}

static void CheckBoolOrDie_(bool ok, const char *what) {
  if (!ok) {
    FatalBoxAndTerminate_(std::string("[GameScene] failed: ") + what);
  }
}

static void CheckHROrDie_(HRESULT hr, const char *what) {
  if (FAILED(hr)) {
    char buf[512];
    sprintf_s(buf, "[GameScene] HRESULT failed: %s (hr=0x%08X)", what,
              (unsigned)hr);
    FatalBoxAndTerminate_(buf);
  }
}

static void CheckFileExists_(const std::string &path) {
  if (!std::filesystem::exists(path)) {
    FatalBoxAndTerminate_(std::string("File not found:\n") + path);
  }
}

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  if (!services_.dx || !services_.input || !services_.audio ||
      !services_.imgui) {
    FatalBoxAndTerminate_(
        "GameScene::Initialize received null service pointer");
  }

  InitLogging_();

  shaderCompiler_.Initialize(services_.dx->GetDXCUtils(),
                             services_.dx->GetDXCCompiler(),
                             services_.dx->GetDXCIncludeHandler());

  InitPipelines_();
  InitResources_();

  camera_ = std::make_unique<DebugCamera>();
  camera_->Initialize();

  InitCamera_();

  accelerationField_.acceleration = {15.0f, 0.0f, 0.0f};
  accelerationField_.area.min = {-1.0f, -1.0f, -1.0f};
  accelerationField_.area.max = {1.0f, 1.0f, 1.0f};
}

void GameScene::Finalize() {
  if (logStream_.is_open()) {
    logStream_.flush();
    logStream_.close();
  }
}

void GameScene::Update() {
  auto *dx = services_.dx;

  if (camera_) {
    camera_->Update(*services_.input);
    view3D_ = camera_->GetViewMatrix();
    proj3D_ = camera_->GetProjectionMatrix();
  } else {
    view3D_ = Inverse(MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
                                       {0.0f, 0.0f, -10.0f}));

    const float w = dx->GetViewport().Width;
    const float h = dx->GetViewport().Height;
    const float aspect = (h != 0.0f) ? (w / h) : 1.0f;

    proj3D_ = MakePerspectiveFovMatrix(0.45f, aspect, 0.1f, 100.0f);
  }

  if (cameraData_) {
    Matrix4x4 invView = Inverse(view3D_);
    cameraData_->worldPosition = {invView.m[3][0], invView.m[3][1],
                                  invView.m[3][2]};
    cameraData_->pad = 0.0f;
  }

#ifdef USE_IMGUI
    static bool settingsOpen = true;
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Always);
  ImGui::Begin("Settings", &settingsOpen);

  if (directionalLightData_) {
    ImGui::SeparatorText("DirectionalLights");

    ImGui::Checkbox("Enable DirectionalLights", &enableDirectionalLight_);
    directionalLightData_->enabled = enableDirectionalLight_ ? 1 : 0;

    int count = directionalLightData_->count;
    ImGui::Text("Count: %d / %d", count, kMaxDirLights);

    if (ImGui::Button("Add DirectionalLight")) {
      if (directionalLightData_->count < kMaxDirLights) {
        int idx = directionalLightData_->count++;
        DirectionalLightCB &dl = directionalLightData_->lights[idx];

        dl.color[0] = 1.0f;
        dl.color[1] = 1.0f;
        dl.color[2] = 1.0f;
        dl.color[3] = 1.0f;
        dl.direction[0] = 0.0f;
        dl.direction[1] = -1.0f;
        dl.direction[2] = 0.0f;
        dl.intensity = 1.0f;
        dl.enabled = enableDirectionalLight_ ? 1 : 0;
        dl.pad0[0] = dl.pad0[1] = dl.pad0[2] = 0.0f;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last##Dir")) {
      if (directionalLightData_->count > 0) {
        directionalLightData_->count--;
        directionalLightData_->lights[directionalLightData_->count].enabled = 0;
      }
    }

    static int editDirIndex = 0;
    if (directionalLightData_->count <= 0)
      editDirIndex = 0;
    else
      editDirIndex =
          std::clamp(editDirIndex, 0, directionalLightData_->count - 1);

    ImGui::SliderInt("Edit Index##Dir", &editDirIndex, 0,
                     std::max(0, directionalLightData_->count - 1));

    if (directionalLightData_->count > 0) {
      DirectionalLightCB &dl = directionalLightData_->lights[editDirIndex];

      ImGui::PushID(editDirIndex);

      bool en = (dl.enabled != 0);
      if (ImGui::Checkbox("Enabled##Dir", &en))
        dl.enabled = en ? 1 : 0;

      ImGui::ColorEdit3("DirColor", dl.color);

      if (ImGui::DragFloat3("DirDirection", dl.direction, 0.01f, -1.0f, 1.0f)) {
        const float x = dl.direction[0];
        const float y = dl.direction[1];
        const float z = dl.direction[2];
        const float lenSq = x * x + y * y + z * z;

        // ゼロに近いとシェーダのnormalizeで不安定になるので保険
        if (lenSq < 1e-6f) {
          dl.direction[0] = 0.0f;
          dl.direction[1] = -1.0f;
          dl.direction[2] = 0.0f;
        }
      }

      ImGui::SliderFloat("DirIntensity", &dl.intensity, 0.0f, 10.0f);

      ImGui::PopID();
    }
  }

  if (pointLightsData_) {
    ImGui::SeparatorText("PointLights");

    ImGui::Checkbox("Enable PointLights", &enablePointLight_);
    pointLightsData_->enabled = enablePointLight_ ? 1 : 0;

    int count = pointLightsData_->count;
    ImGui::Text("Count: %d / %d", count, kMaxPointLights);

    if (ImGui::Button("Add PointLight")) {
      if (pointLightsData_->count < kMaxPointLights) {
        int idx = pointLightsData_->count++;
        PointLightCB &pl = pointLightsData_->lights[idx];

        pl.color[0] = 1.0f;
        pl.color[1] = 1.0f;
        pl.color[2] = 1.0f;
        pl.color[3] = 1.0f;
        pl.position[0] = float(idx) * 2.0f;
        pl.position[1] = 2.0f;
        pl.position[2] = -2.0f;
        pl.intensity = 1.0f;
        pl.radius = 10.0f;
        pl.decay = 2.0f;
        pl.enabled = enablePointLight_ ? 1 : 0;
        pl.pad0 = 0.0f;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last")) {
      if (pointLightsData_->count > 0) {
        pointLightsData_->count--;
        pointLightsData_->lights[pointLightsData_->count].enabled = 0;
      }
    }

    static int editIndex = 0;
    if (pointLightsData_->count <= 0)
      editIndex = 0;
    else
      editIndex = std::clamp(editIndex, 0, pointLightsData_->count - 1);

    ImGui::SliderInt("Edit Index", &editIndex, 0,
                     std::max(0, pointLightsData_->count - 1));

    if (pointLightsData_->count > 0) {
      PointLightCB &pl = pointLightsData_->lights[editIndex];

      // 将来、複数ライトを同時表示するならPushIDが安全
      ImGui::PushID(editIndex);

      bool en = (pl.enabled != 0);
      if (ImGui::Checkbox("Enabled", &en))
        pl.enabled = en ? 1 : 0;

      ImGui::ColorEdit3("PointColor", pl.color);
      ImGui::DragFloat3("PointPosition", pl.position, 0.01f);
      ImGui::SliderFloat("PointIntensity", &pl.intensity, 0.0f, 10.0f);
      ImGui::SliderFloat("PointRadius", &pl.radius, 0.01f, 50.0f);
      ImGui::SliderFloat("PointDecay", &pl.decay, 0.01f, 8.0f);

      ImGui::PopID();
    }
  }

  if (spotLightData_) {
    ImGui::SeparatorText("SpotLights");

    ImGui::Checkbox("Enable SpotLights", &enableSpotLight_);
    spotLightData_->enabled = enableSpotLight_ ? 1 : 0;

    int count = spotLightData_->count;
    ImGui::Text("Count: %d / %d", count, kMaxSpotLights);

    if (ImGui::Button("Add SpotLight")) {
      if (spotLightData_->count < kMaxSpotLights) {
        int idx = spotLightData_->count++;
        SpotLightCB &sl = spotLightData_->lights[idx];

        std::memset(&sl, 0, sizeof(SpotLightCB));

        sl.color[0] = 1.0f;
        sl.color[1] = 1.0f;
        sl.color[2] = 1.0f;
        sl.color[3] = 1.0f;
        sl.position[0] = float(idx) * 2.0f;
        sl.position[1] = 3.0f;
        sl.position[2] = -2.0f;
        sl.intensity = 1.0f;

        sl.direction[0] = 0.0f;
        sl.direction[1] = -1.0f;
        sl.direction[2] = 0.0f;

        sl.distance = 10.0f;
        sl.decay = 2.0f;

        const float pi = 3.14159265f;
        float angleRad = 30.0f * (pi / 180.0f);
        sl.cosAngle = std::cos(angleRad);

        sl.enabled = enableSpotLight_ ? 1 : 0;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last##Spot")) {
      if (spotLightData_->count > 0) {
        spotLightData_->count--;
        spotLightData_->lights[spotLightData_->count].enabled = 0;
      }
    }

    static int editSpotIndex = 0;
    if (spotLightData_->count <= 0)
      editSpotIndex = 0;
    else
      editSpotIndex = std::clamp(editSpotIndex, 0, spotLightData_->count - 1);

    ImGui::SliderInt("Edit Index##Spot", &editSpotIndex, 0,
                     std::max(0, spotLightData_->count - 1));

    if (spotLightData_->count > 0) {
      SpotLightCB &sl = spotLightData_->lights[editSpotIndex];

      ImGui::PushID(editSpotIndex);

      bool en = (sl.enabled != 0);
      if (ImGui::Checkbox("Enabled##Spot", &en))
        sl.enabled = en ? 1 : 0;

      ImGui::ColorEdit3("SpotColor", sl.color);
      ImGui::DragFloat3("SpotPosition", sl.position, 0.01f);

      if (ImGui::DragFloat3("SpotDirection", sl.direction, 0.01f, -1.0f,
                            1.0f)) {
        const float x = sl.direction[0];
        const float y = sl.direction[1];
        const float z = sl.direction[2];
        const float lenSq = x * x + y * y + z * z;

        if (lenSq < 1e-6f) {
          sl.direction[0] = 0.0f;
          sl.direction[1] = -1.0f;
          sl.direction[2] = 0.0f;
        }
      }

      ImGui::SliderFloat("SpotIntensity", &sl.intensity, 0.0f, 10.0f);
      ImGui::SliderFloat("SpotDistance", &sl.distance, 0.01f, 50.0f);
      ImGui::SliderFloat("SpotDecay", &sl.decay, 0.01f, 8.0f);

      const float pi = 3.14159265f;
      float cosA = std::max(-1.0f, std::min(1.0f, sl.cosAngle));
      float coneDeg = std::acos(cosA) * (180.0f / pi);
      if (ImGui::SliderFloat("Cone Angle (deg)", &coneDeg, 1.0f, 89.0f)) {
        float rad = coneDeg * (pi / 180.0f);
        sl.cosAngle = std::cos(rad);
      }

      ImGui::PopID();
    }
  }
  ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Always);
  ImGui::Begin("Object", &settingsOpen);
  ImGui::Separator();
  static float sphereCol[3] = {1.0f, 1.0f, 1.0f};
  ImGui::Text("ObjectColor");
  if (ImGui::ColorEdit3("SphereColor", sphereCol)) {
    modelSphere_.SetColor({sphereCol[0], sphereCol[1], sphereCol[2], 1.0f});
  }

  static float planeCol[3] = {1.0f, 1.0f, 1.0f};
  if (ImGui::ColorEdit3("PlaneColor", planeCol)) {
    modelPlane_.SetColor({planeCol[0], planeCol[1], planeCol[2], 1.0f});
  }

  static float spriteCol[3] = {1.0f, 1.0f, 1.0f};
  if (ImGui::ColorEdit3("SpriteColor", spriteCol)) {
    sprite_.SetColor({spriteCol[0], spriteCol[1], spriteCol[2], 1.0f});
  }

  ImGui::Separator();
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

  /*ImGui::Begin("Particles");

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
  }*/

  /*{
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
  }*/

  /*ImGui::SeparatorText("Manager");

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

  ImGui::End();*/
#endif

  const float deltaTime = 1.0f / 60.0f;

  particleEmitter_.Update(deltaTime);

  ParticleManager::GetInstance()->SetEnableAccelerationField(
      enableAccelerationField_);
  ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);
  ParticleManager::GetInstance()->Update(view3D_, proj3D_, deltaTime);
}

void GameScene::Draw(ID3D12GraphicsCommandList *cmdList) {
  auto *dx = services_.dx;
  (void)dx;

  Matrix4x4 viewMatrix = view3D_;
  Matrix4x4 projectionMatrix = proj3D_;

  {
    Matrix4x4 worldSphere = MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate);
    modelSphere_.SetWorld(worldSphere);
    modelSphere_.SetLightingMode(lightingMode_);
    modelSphere_.SetSpecularColor({1.0f, 1.0f, 1.0f});
    modelSphere_.SetShininess(64.0f);

    Matrix4x4 worldPlane = MakeAffineMatrix(
        transform2_.scale, transform2_.rotate, transform2_.translate);
    modelPlane_.SetWorld(worldPlane);

    terrainTransform_.translate = {0.0f, -3.0f, 0.0f};
    Matrix4x4 worldTerrain =
        MakeAffineMatrix(terrainTransform_.scale, terrainTransform_.rotate,
                         terrainTransform_.translate);
    modelTerrain_.SetWorld(worldTerrain);

    cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
    cmdList->SetPipelineState(objPipeline_.GetPipelineState());

    modelSphere_.Draw(cmdList,viewMatrix, projectionMatrix, directionalLightCB_.Get(),
                      cameraCB_.Get(), pointLightCB_.Get(), spotLightCB_.Get());
    modelPlane_.Draw(cmdList, viewMatrix, projectionMatrix,
                     directionalLightCB_.Get(),
                     cameraCB_.Get(), pointLightCB_.Get(), spotLightCB_.Get());
    modelTerrain_.Draw(cmdList, viewMatrix, projectionMatrix,
                       directionalLightCB_.Get(),
                       cameraCB_.Get(), pointLightCB_.Get(),
                       spotLightCB_.Get());
  }

  if (showEmitterGizmo_) {
    const auto &ep = particleEmitter_.GetParams();

    cmdList->SetGraphicsRootSignature(
        emitterGizmoPipelineWire_.GetRootSignature());
    cmdList->SetPipelineState(emitterGizmoPipelineWire_.GetPipelineState());

    if (ep.shape == EmitterShape::Box) {
      Vector3 scale = {ep.extent.x * 2.0f, ep.extent.y * 2.0f,
                       ep.extent.z * 2.0f};
      Matrix4x4 world =
          MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterBox_.SetWorld(world);
      modelEmitterBox_.Draw(cmdList, viewMatrix, projectionMatrix,
                            directionalLightCB_.Get(), cameraCB_.Get(),
                            pointLightCB_.Get(), spotLightCB_.Get());
    } else {
      Vector3 scale = {std::max(ep.extent.x, 0.001f),
                       std::max(ep.extent.y, 0.001f),
                       std::max(ep.extent.z, 0.001f)};
      Matrix4x4 world =
          MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterSphere_.SetWorld(world);
      modelEmitterSphere_.Draw(cmdList, viewMatrix, projectionMatrix,
                               directionalLightCB_.Get(), cameraCB_.Get(),
                               pointLightCB_.Get(), spotLightCB_.Get());
    }
  }

  {
    Matrix4x4 invView = Inverse(viewMatrix);
    Vector3 camPos = {invView.m[3][0], invView.m[3][1], invView.m[3][2]};

    skybox_.Update(viewMatrix, projectionMatrix, camPos,
                   {100.0f, 100.0f, 100.0f});
    skybox_.Draw(cmdList);
  }

#ifdef USE_IMGUI
  services_.imgui->Draw(cmdList);
#endif
}

void GameScene::InitLogging_() {
  std::filesystem::create_directory("logs");

  auto now = std::chrono::system_clock::now();
  auto nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
  std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};
  std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
  std::string logFilePath = std::string("logs/") + dateString + ".log";
  logStream_.open(logFilePath);
}

void GameScene::InitPipelines_() {
  auto *device = services_.dx->GetDevice();
  auto *dxcUtils = services_.dx->GetDXCUtils();
  auto *dxcCompiler = services_.dx->GetDXCCompiler();
  auto *includeHandler = services_.dx->GetDXCIncludeHandler();

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

  PipelineDesc skyboxDesc = UnifiedPipeline::MakeSkyboxDesc();
  CheckBoolOrDie_(skyboxPipeline_.Initialize(device, dxcUtils, dxcCompiler,
                                             includeHandler, skyboxDesc),
                  "skyboxPipeline_.Initialize");
}

void GameScene::InitResources_() {
  auto *device = services_.dx->GetDevice();

  ModelManager::GetInstance()->Initialize(services_.dx);

  resSphere_ = ModelManager::GetInstance()->Load("resources/sphere/sphere.obj");
  resPlane_ = ModelManager::GetInstance()->Load("resources/plane/plane.gltf");
  resCube_ = ModelManager::GetInstance()->Load("resources/cube/cube.obj");
  resTerrain_ =
      ModelManager::GetInstance()->Load("resources/terrain/terrain.obj");

  CheckFileExists_("resources/particle/circle.png");
  CheckFileExists_("resources/sound/select.mp3");

  {
    ModelInstance::CreateInfo ci{};
    ci.dx = services_.dx;
    ci.resource = resSphere_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelSphere_.Initialize(ci), "modelSphere_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.dx = services_.dx;
    ci.resource = resPlane_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelPlane_.Initialize(ci), "modelPlane_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.dx = services_.dx;
    ci.resource = resSphere_;
    ci.baseColor = {0.3f, 0.8f, 1.0f, 0.3f};
    ci.lightingMode = 0;
    CheckBoolOrDie_(modelEmitterSphere_.Initialize(ci),
                    "modelEmitterSphere_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.dx = services_.dx;
    ci.resource = resCube_;
    ci.baseColor = {1.0f, 0.8f, 0.2f, 0.3f};
    ci.lightingMode = 0;
    CheckBoolOrDie_(modelEmitterBox_.Initialize(ci),
                    "modelEmitterBox_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.dx = services_.dx;
    ci.resource = resTerrain_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelTerrain_.Initialize(ci), "modelTerrain_.Initialize");
  }

  {
    Sprite::CreateInfo sprInfo{};
    sprInfo.dx = services_.dx;
    sprInfo.texturePath = "resources/dds/dds.dds";
    sprInfo.size = {640.0f, 360.0f};
    sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};

    bool spriteInitOk = sprite_.Initialize(sprInfo);
    OutputDebugStringA(spriteInitOk
                           ? "[DDS CHECK] sprite_.Initialize success\n"
                           : "[DDS CHECK] sprite_.Initialize failed\n");

    CheckBoolOrDie_(spriteInitOk, "sprite_.Initialize");
  }

  {
    CheckBoolOrDie_(ParticleManager::GetInstance()->CreateParticleGroup(
                        particleGroupName_, "resources/particle/circle.png",
                        kParticleCount_),
                    "ParticleManager::CreateParticleGroup");

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

  {
    CheckBoolOrDie_(
        skybox_.Initialize(services_.dx, &skyboxPipeline_,
                                       "resources/dds/dds.dds"),
        "skybox_.Initialize");
  }

  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = Align256_(sizeof(DirectionalLightGroupCB));
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

    std::memset(directionalLightData_, 0, sizeof(DirectionalLightGroupCB));
    directionalLightData_->enabled = 0;
    directionalLightData_->count = 1;

    DirectionalLightCB &dl0 = directionalLightData_->lights[0];
    dl0.color[0] = 1.0f;
    dl0.color[1] = 1.0f;
    dl0.color[2] = 1.0f;
    dl0.color[3] = 1.0f;
    dl0.direction[0] = 0.0f;
    dl0.direction[1] = -1.0f;
    dl0.direction[2] = 0.0f;
    dl0.intensity = 1.0f;
    dl0.enabled = 1;

    enableDirectionalLight_ = false;
  }

  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = Align256_(sizeof(CameraForGPU));
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cameraCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(cameraCB_)");

    hr = cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));
    CheckHROrDie_(hr, "cameraCB_->Map");

    cameraData_->worldPosition = {0.0f, 0.0f, -10.0f};
    cameraData_->pad = 0.0f;
  }

  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = Align256_(sizeof(PointLightGroupCB));
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&pointLightCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(pointLightCB_)");

    hr = pointLightCB_->Map(0, nullptr,
                            reinterpret_cast<void **>(&pointLightsData_));
    CheckHROrDie_(hr, "pointLightCB_->Map");

    std::memset(pointLightsData_, 0, sizeof(PointLightGroupCB));
    pointLightsData_->enabled = 1;
    pointLightsData_->count = 1;

    PointLightCB &pl0 = pointLightsData_->lights[0];
    pl0.color[0] = 1.0f;
    pl0.color[1] = 1.0f;
    pl0.color[2] = 1.0f;
    pl0.color[3] = 1.0f;
    pl0.position[0] = 0.0f;
    pl0.position[1] = 2.0f;
    pl0.position[2] = -2.0f;
    pl0.intensity = 1.0f;
    pl0.radius = 10.0f;
    pl0.decay = 2.0f;
    pl0.enabled = 1;
    pl0.pad0 = 0.0f;

    enablePointLight_ = true;
  }

  {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = Align256_(sizeof(SpotLightGroupCB));
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&spotLightCB_));
    CheckHROrDie_(hr, "CreateCommittedResource(spotLightCB_)");

    hr = spotLightCB_->Map(0, nullptr,
                           reinterpret_cast<void **>(&spotLightData_));
    CheckHROrDie_(hr, "spotLightCB_->Map");

    std::memset(spotLightData_, 0, sizeof(SpotLightGroupCB));
    spotLightData_->enabled = 0;
    spotLightData_->count = 1;

    SpotLightCB &sl0 = spotLightData_->lights[0];
    sl0.color[0] = 1.0f;
    sl0.color[1] = 1.0f;
    sl0.color[2] = 1.0f;
    sl0.color[3] = 1.0f;
    sl0.position[0] = 0.0f;
    sl0.position[1] = 3.0f;
    sl0.position[2] = -2.0f;
    sl0.intensity = 1.0f;
    sl0.direction[0] = 0.0f;
    sl0.direction[1] = -1.0f;
    sl0.direction[2] = 0.0f;
    sl0.distance = 10.0f;
    sl0.decay = 2.0f;

    const float pi = 3.14159265f;
    float angleRad = 30.0f * (pi / 180.0f);
    sl0.cosAngle = std::cos(angleRad);
    sl0.enabled = 1;

    enableSpotLight_ = false;
  }

  {
    const bool ok =
        services_.audio->Load("select", L"resources/sound/select.mp3", 1.0f);
    CheckBoolOrDie_(ok, "audio->Load(select.mp3)");
  }
}

void GameScene::InitCamera_() {
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

  const float w = services_.dx->GetViewport().Width;
  const float h = services_.dx->GetViewport().Height;
  const float aspect = (h != 0.0f) ? (w / h) : 1.0f;

  if (camera_) {
    camera_->SetPerspective(0.45f, aspect, 0.1f, 100.0f);
  }
}
