#define NOMINMAX
#include "GameScene.h"

#include "DebugCamera.h"
#include "GameCamera.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Renderer.h"
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

static void FatalBoxAndTerminate_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Fatal", MB_OK | MB_ICONERROR);
  std::terminate();
}

static void CheckBoolOrDie_(bool ok, const char *what) {
  if (!ok) {
    FatalBoxAndTerminate_(std::string("[GameScene] failed: ") + what);
  }
}

static void CheckFileExists_(const std::string &path) {
  if (!std::filesystem::exists(path)) {
    FatalBoxAndTerminate_(std::string("File not found:\n") + path);
  }
}

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  if (!services_.input || !services_.audio) {
    FatalBoxAndTerminate_(
        "GameScene::Initialize received null service pointer");
  }

  InitLogging_();

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
  if (camera_) {
    camera_->Update(*services_.input);
  }

#ifdef USE_IMGUI
  static bool settingsOpen = true;
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Always);
  ImGui::Begin("Settings", &settingsOpen);

  // ===== DirectionalLights =====
  {
    ImGui::SeparatorText("DirectionalLights");

    ImGui::Checkbox("Enable DirectionalLights", &enableDirectionalLight_);

    int count = static_cast<int>(dirLights_.size());
    ImGui::Text("Count: %d / %d", count, kMaxDirLights);

    if (ImGui::Button("Add DirectionalLight")) {
      if (count < kMaxDirLights) {
        DirLight dl{};
        dl.color = {1.0f, 1.0f, 1.0f};
        dl.direction = {0.0f, -1.0f, 0.0f};
        dl.intensity = 1.0f;
        dl.enabled = enableDirectionalLight_;
        dirLights_.push_back(dl);
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last##Dir")) {
      if (!dirLights_.empty()) {
        dirLights_.pop_back();
      }
    }

    static int editDirIndex = 0;
    if (dirLights_.empty())
      editDirIndex = 0;
    else
      editDirIndex =
          std::clamp(editDirIndex, 0, static_cast<int>(dirLights_.size()) - 1);

    ImGui::SliderInt("Edit Index##Dir", &editDirIndex, 0,
                     std::max(0, static_cast<int>(dirLights_.size()) - 1));

    if (!dirLights_.empty()) {
      DirLight &dl = dirLights_[editDirIndex];

      ImGui::PushID(editDirIndex);

      ImGui::Checkbox("Enabled##Dir", &dl.enabled);

      float col[3] = {dl.color.x, dl.color.y, dl.color.z};
      if (ImGui::ColorEdit3("DirColor", col)) {
        dl.color = {col[0], col[1], col[2]};
      }

      float dir[3] = {dl.direction.x, dl.direction.y, dl.direction.z};
      if (ImGui::DragFloat3("DirDirection", dir, 0.01f, -1.0f, 1.0f)) {
        const float lenSq = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
        if (lenSq < 1e-6f) {
          dir[0] = 0.0f; dir[1] = -1.0f; dir[2] = 0.0f;
        }
        dl.direction = {dir[0], dir[1], dir[2]};
      }

      ImGui::SliderFloat("DirIntensity", &dl.intensity, 0.0f, 10.0f);

      ImGui::PopID();
    }
  }

  // ===== PointLights =====
  {
    ImGui::SeparatorText("PointLights");

    ImGui::Checkbox("Enable PointLights", &enablePointLight_);

    int count = static_cast<int>(pointLights_.size());
    ImGui::Text("Count: %d / %d", count, kMaxPointLights);

    if (ImGui::Button("Add PointLight")) {
      if (count < kMaxPointLights) {
        PointLight pl{};
        pl.color = {1.0f, 1.0f, 1.0f};
        pl.position = {static_cast<float>(count) * 2.0f, 2.0f, -2.0f};
        pl.intensity = 1.0f;
        pl.radius = 10.0f;
        pl.decay = 2.0f;
        pl.enabled = enablePointLight_;
        pointLights_.push_back(pl);
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last")) {
      if (!pointLights_.empty()) {
        pointLights_.pop_back();
      }
    }

    static int editIndex = 0;
    if (pointLights_.empty())
      editIndex = 0;
    else
      editIndex = std::clamp(editIndex, 0, static_cast<int>(pointLights_.size()) - 1);

    ImGui::SliderInt("Edit Index", &editIndex, 0,
                     std::max(0, static_cast<int>(pointLights_.size()) - 1));

    if (!pointLights_.empty()) {
      PointLight &pl = pointLights_[editIndex];

      ImGui::PushID(editIndex);

      ImGui::Checkbox("Enabled", &pl.enabled);

      float col[3] = {pl.color.x, pl.color.y, pl.color.z};
      if (ImGui::ColorEdit3("PointColor", col)) {
        pl.color = {col[0], col[1], col[2]};
      }

      float pos[3] = {pl.position.x, pl.position.y, pl.position.z};
      if (ImGui::DragFloat3("PointPosition", pos, 0.01f)) {
        pl.position = {pos[0], pos[1], pos[2]};
      }

      ImGui::SliderFloat("PointIntensity", &pl.intensity, 0.0f, 10.0f);
      ImGui::SliderFloat("PointRadius", &pl.radius, 0.01f, 50.0f);
      ImGui::SliderFloat("PointDecay", &pl.decay, 0.01f, 8.0f);

      ImGui::PopID();
    }
  }

  // ===== SpotLights =====
  {
    ImGui::SeparatorText("SpotLights");

    ImGui::Checkbox("Enable SpotLights", &enableSpotLight_);

    int count = static_cast<int>(spotLights_.size());
    ImGui::Text("Count: %d / %d", count, kMaxSpotLights);

    if (ImGui::Button("Add SpotLight")) {
      if (count < kMaxSpotLights) {
        SpotLight sl{};
        sl.color = {1.0f, 1.0f, 1.0f};
        sl.position = {static_cast<float>(count) * 2.0f, 3.0f, -2.0f};
        sl.direction = {0.0f, -1.0f, 0.0f};
        sl.intensity = 1.0f;
        sl.distance = 10.0f;
        sl.decay = 2.0f;
        sl.coneAngleDeg = 30.0f;
        sl.enabled = enableSpotLight_;
        spotLights_.push_back(sl);
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Last##Spot")) {
      if (!spotLights_.empty()) {
        spotLights_.pop_back();
      }
    }

    static int editSpotIndex = 0;
    if (spotLights_.empty())
      editSpotIndex = 0;
    else
      editSpotIndex = std::clamp(editSpotIndex, 0, static_cast<int>(spotLights_.size()) - 1);

    ImGui::SliderInt("Edit Index##Spot", &editSpotIndex, 0,
                     std::max(0, static_cast<int>(spotLights_.size()) - 1));

    if (!spotLights_.empty()) {
      SpotLight &sl = spotLights_[editSpotIndex];

      ImGui::PushID(editSpotIndex);

      ImGui::Checkbox("Enabled##Spot", &sl.enabled);

      float col[3] = {sl.color.x, sl.color.y, sl.color.z};
      if (ImGui::ColorEdit3("SpotColor", col)) {
        sl.color = {col[0], col[1], col[2]};
      }

      float pos[3] = {sl.position.x, sl.position.y, sl.position.z};
      if (ImGui::DragFloat3("SpotPosition", pos, 0.01f)) {
        sl.position = {pos[0], pos[1], pos[2]};
      }

      float dir[3] = {sl.direction.x, sl.direction.y, sl.direction.z};
      if (ImGui::DragFloat3("SpotDirection", dir, 0.01f, -1.0f, 1.0f)) {
        const float lenSq = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
        if (lenSq < 1e-6f) {
          dir[0] = 0.0f; dir[1] = -1.0f; dir[2] = 0.0f;
        }
        sl.direction = {dir[0], dir[1], dir[2]};
      }

      ImGui::SliderFloat("SpotIntensity", &sl.intensity, 0.0f, 10.0f);
      ImGui::SliderFloat("SpotDistance", &sl.distance, 0.01f, 50.0f);
      ImGui::SliderFloat("SpotDecay", &sl.decay, 0.01f, 8.0f);
      ImGui::SliderFloat("Cone Angle (deg)", &sl.coneAngleDeg, 1.0f, 89.0f);

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

  // Transform Camera override
  if (useTransformCamera && camera_) {
    // カメラを手動トランスフォームで上書き（デバッグ用）
    // DebugCamera の Update を無視する形
  }

#endif

  const float deltaTime = 1.0f / 60.0f;

  particleEmitter_.Update(deltaTime);

  ParticleManager::GetInstance()->SetEnableAccelerationField(
      enableAccelerationField_);
  ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);
  ParticleManager::GetInstance()->Update(deltaTime);
}

void GameScene::Draw() {
  auto* renderer = Renderer::GetInstance();

  // カメラ設定（view/proj + カメラ位置を一括転送）
  if (camera_) {
    renderer->SetCamera(*camera_);
  }

  // ライト設定（高レベル記述子 → 内部で CB 変換）
  renderer->SetDirectionalLights(dirLights_, enableDirectionalLight_);
  renderer->SetPointLights(pointLights_, enablePointLight_);
  renderer->SetSpotLights(spotLights_, enableSpotLight_);

  // 3D モデル描画
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

    modelSphere_.Draw();
    modelPlane_.Draw();
    modelTerrain_.Draw();
  }

  //sprite_.Draw();

  if (showEmitterGizmo_) {
    const auto &ep = particleEmitter_.GetParams();

    if (ep.shape == EmitterShape::Box) {
      Vector3 scale = {ep.extent.x * 2.0f, ep.extent.y * 2.0f,
                       ep.extent.z * 2.0f};
      Matrix4x4 world =
          MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterBox_.SetWorld(world);
      modelEmitterBox_.SetWireframe(true);
      modelEmitterBox_.Draw();
    } else {
      Vector3 scale = {std::max(ep.extent.x, 0.001f),
                       std::max(ep.extent.y, 0.001f),
                       std::max(ep.extent.z, 0.001f)};
      Matrix4x4 world =
          MakeAffineMatrix(scale, {0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterSphere_.SetWorld(world);
      modelEmitterSphere_.SetWireframe(true);
      modelEmitterSphere_.Draw();
    }
  }

  // Skybox
  {
    Matrix4x4 viewMatrix = renderer->GetViewMatrix();
    Matrix4x4 projMatrix = renderer->GetProjectionMatrix();
    Matrix4x4 invView = Inverse(viewMatrix);
    Vector3 camPos = {invView.m[3][0], invView.m[3][1], invView.m[3][2]};

    skybox_.Update(viewMatrix, projMatrix, camPos, {100.0f, 100.0f, 100.0f});
    skybox_.Draw();
  }
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

void GameScene::InitResources_() {
  resSphere_ = ModelManager::GetInstance()->Load("resources/sphere/sphere.obj");
  resPlane_ = ModelManager::GetInstance()->Load("resources/plane/plane.gltf");
  resCube_ = ModelManager::GetInstance()->Load("resources/cube/cube.obj");
  resTerrain_ =
      ModelManager::GetInstance()->Load("resources/terrain/terrain.obj");

  CheckFileExists_("resources/particle/circle.png");
  CheckFileExists_("resources/sound/select.mp3");

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resSphere_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelSphere_.Initialize(ci), "modelSphere_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resPlane_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelPlane_.Initialize(ci), "modelPlane_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resSphere_;
    ci.baseColor = {0.3f, 0.8f, 1.0f, 0.3f};
    ci.lightingMode = 0;
    CheckBoolOrDie_(modelEmitterSphere_.Initialize(ci),
                    "modelEmitterSphere_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resCube_;
    ci.baseColor = {1.0f, 0.8f, 0.2f, 0.3f};
    ci.lightingMode = 0;
    CheckBoolOrDie_(modelEmitterBox_.Initialize(ci),
                    "modelEmitterBox_.Initialize");
  }

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resTerrain_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    CheckBoolOrDie_(modelTerrain_.Initialize(ci), "modelTerrain_.Initialize");
  }

  {
    Sprite::CreateInfo sprInfo{};
    sprInfo.texturePath = "resources/plane/uvChecker.png";
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
        skybox_.Initialize("resources/dds/dds.dds"),
        "skybox_.Initialize");
  }

  // ライト初期値
  {
    DirLight dl{};
    dl.color = {1.0f, 1.0f, 1.0f};
    dl.direction = {0.0f, -1.0f, 0.0f};
    dl.intensity = 1.0f;
    dl.enabled = true;
    dirLights_.push_back(dl);
    enableDirectionalLight_ = false;
  }

  {
    PointLight pl{};
    pl.color = {1.0f, 1.0f, 1.0f};
    pl.position = {0.0f, 2.0f, -2.0f};
    pl.intensity = 1.0f;
    pl.radius = 10.0f;
    pl.decay = 2.0f;
    pl.enabled = true;
    pointLights_.push_back(pl);
    enablePointLight_ = true;
  }

  {
    SpotLight sl{};
    sl.color = {1.0f, 1.0f, 1.0f};
    sl.position = {0.0f, 3.0f, -2.0f};
    sl.direction = {0.0f, -1.0f, 0.0f};
    sl.intensity = 1.0f;
    sl.distance = 10.0f;
    sl.decay = 2.0f;
    sl.coneAngleDeg = 30.0f;
    sl.enabled = true;
    spotLights_.push_back(sl);
    enableSpotLight_ = false;
  }

  /*{
    const bool ok =
        services_.audio->Load("select", L"resources/sound/select.mp3", 1.0f);
    CheckBoolOrDie_(ok, "audio->Load(select.mp3)");
  }*/
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

  const float aspect = Renderer::GetInstance()->GetAspectRatio();

  if (camera_) {
    camera_->SetPerspective(0.45f, aspect, 0.1f, 100.0f);
  }
}
