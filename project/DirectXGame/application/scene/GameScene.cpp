#define NOMINMAX
#include "GameScene.h"
#include "DebugCamera.h"
#include "DirectXCommon.h"
#include "Renderer.h"
#include "TextureResource.h"
#include "graphics/texture/TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "loader/LevelLoader.h"

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

  // レベルの読み込み
  LoadLevel_("level");
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

  ImGui::Begin("Object", &settingsOpen);
  ImGui::Separator();

  // 反射設定
  ImGui::SeparatorText("Environment Reflection");
  ImGui::Checkbox("Enable Reflection", &enableReflection_);
  if (enableReflection_) {
    ImGui::SliderFloat("Weight", &reflectionWeight_, 0.0f, 1.0f);
  }

  static float sphereCol[3] = {1.0f, 1.0f, 1.0f};
  ImGui::Text("ObjectColor");
  if (ImGui::ColorEdit3("SphereColor", sphereCol)) {
    modelSphere_.SetColor({sphereCol[0], sphereCol[1], sphereCol[2], 1.0f});
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
  ImGui::Combo("Particle Blend", &particleBlendMode_, blendModeItems,
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
  if (ImGui::CollapsingHeader("Ring Primitive")) {
      bool changed = false;
      int divide = static_cast<int>(ringParams_.divide);
      if (ImGui::SliderInt("Divide", &divide, 3, 128)) {
          ringParams_.divide = static_cast<uint32_t>(divide);
          changed = true;
      }
      if (ImGui::SliderFloat("Outer Radius", &ringParams_.outerRadius, 0.1f, 10.0f)) {
          if (ringParams_.outerRadius < ringParams_.innerRadius) {
              ringParams_.outerRadius = ringParams_.innerRadius + 0.01f;
          }
          changed = true;
      }
      if (ImGui::SliderFloat("Inner Radius", &ringParams_.innerRadius, 0.0f, 10.0f)) {
          if (ringParams_.innerRadius > ringParams_.outerRadius) {
              ringParams_.innerRadius = ringParams_.outerRadius - 0.01f;
          }
          changed = true;
      }
      changed |= ImGui::SliderAngle("Start Angle", &ringParams_.startAngle, -360.0f, 360.0f);
      changed |= ImGui::SliderAngle("End Angle", &ringParams_.endAngle, -360.0f, 360.0f);
      changed |= ImGui::Checkbox("UV Vertical", &ringParams_.uvVertical);
      changed |= ImGui::ColorEdit4("Color Inner", &ringParams_.colorInner.x);
      changed |= ImGui::ColorEdit4("Color Outer", &ringParams_.colorOuter.x);
      changed |= ImGui::SliderFloat("Alpha Ref##Ring", &ringParams_.alphaReference, 0.0f, 1.0f);
      
      ImGui::DragFloat2("UV Scale##Ring", &ringUVScale_.x, 0.1f);

      if (changed) {
          auto* dx = Renderer::GetInstance()->GetDX();
          ring_.Update(dx->GetDevice(), ringParams_);
      }
      
      ImGui::DragFloat3("Ring Pos", &ringTransform_.translate.x, 0.1f);
      ImGui::DragFloat3("Ring Rot", &ringTransform_.rotate.x, 0.05f);
      
      static bool autoRotate = true;
      ImGui::Checkbox("Auto Rotate", &autoRotate);
      if (autoRotate) {
          ringTransform_.rotate.z += 1.0f * deltaTime;
      }
  }

  if (ImGui::CollapsingHeader("Cylinder Primitive")) {
      bool changed = false;
      int divide = static_cast<int>(cylinderParams_.divide);
      if (ImGui::SliderInt("Divide##Cyl", &divide, 3, 128)) {
          cylinderParams_.divide = static_cast<uint32_t>(divide);
          changed = true;
      }
      changed |= ImGui::DragFloat2("Top Radius (X,Z)", &cylinderParams_.topRadiusX, 0.1f, 0.0f, 10.0f);
      changed |= ImGui::DragFloat2("Bottom Radius (X,Z)", &cylinderParams_.bottomRadiusX, 0.1f, 0.0f, 10.0f);
      changed |= ImGui::SliderFloat("Height##Cyl", &cylinderParams_.height, 0.1f, 10.0f);
      changed |= ImGui::SliderAngle("Start Angle##Cyl", &cylinderParams_.startAngle, -360.0f, 360.0f);
      changed |= ImGui::SliderAngle("End Angle##Cyl", &cylinderParams_.endAngle, -360.0f, 360.0f);
      changed |= ImGui::Checkbox("Flip V##Cyl", &cylinderParams_.flipV);
      changed |= ImGui::Checkbox("UV Vertical##Cyl", &cylinderParams_.uvVertical);
      changed |= ImGui::SliderFloat("Alpha Reference", &cylinderParams_.alphaReference, 0.0f, 1.0f);
      changed |= ImGui::ColorEdit4("Color Top", &cylinderParams_.colorTop.x);
      changed |= ImGui::ColorEdit4("Color Bottom", &cylinderParams_.colorBottom.x);

      ImGui::DragFloat2("UV Scale##Cyl", &cylinderUVScale_.x, 0.1f);

      if (changed) {
          auto* dx = Renderer::GetInstance()->GetDX();
          cylinder_.Update(dx->GetDevice(), cylinderParams_);
      }
      
      ImGui::DragFloat3("Cylinder Pos", &cylinderTransform_.translate.x, 0.1f);
      ImGui::DragFloat3("Cylinder Rot", &cylinderTransform_.rotate.x, 0.05f);
  }

  ParticleManager::GetInstance()->Update(deltaTime);

  // レベルオブジェクトの更新
  for (auto& obj : levelObjects_) {
      obj->Update();
  }

  // テスト用：スペースキーで原点にエフェクト発生
  if (services_.input->TriggerKey(DIK_SPACE)) {
    SpawnHitEffect({0.0f, 0.0f, -1.0f});
  }

  // エフェクトの更新
  for (auto &ef : hitEffects_) {
    if (!ef.isActive)
      continue;

    ef.frame += 1.0f;
    float t = ef.frame / ef.maxFrame; // 0.0 ~ 1.0

    // スケール：時間とともに大きく
    float scaleVal = t * 5.0f;
    Matrix4x4 world = MakeAffineMatrix({scaleVal, scaleVal, scaleVal},
                                       {0, 0, 0}, ef.position);
    ef.instance.SetWorld(world);

    // 透明度：時間とともに消える
    ef.instance.SetColor({1.0f, 1.0f, 1.0f, 1.0f - t});

    if (ef.frame >= ef.maxFrame)
      ef.isActive = false;
  }

  // 不要なエフェクトを削除
  hitEffects_.erase(
      std::remove_if(hitEffects_.begin(), hitEffects_.end(),
                     [](const HitEffect &e) { return !e.isActive; }),
      hitEffects_.end());
}

void GameScene::Draw() {
  auto* renderer = Renderer::GetInstance();

  // カメラ設定（view/proj + カメラ位置を一括転送）
  if (camera_) {
    renderer->SetCamera(*camera_);
  }

  // 環境マップをRendererにセット (Skyboxのテクスチャを流用)
  renderer->SetEnvironmentMap(skybox_.GetTexture());

  // ライト設定（高レベル記述子 → 内部で CB 変換）
  renderer->SetDirectionalLights(dirLights_, enableDirectionalLight_);
  renderer->SetPointLights(pointLights_, enablePointLight_);
  renderer->SetSpotLights(spotLights_, enableSpotLight_);

  // 3D モデル描画
  //{
  //  Matrix4x4 worldSphere = MakeAffineMatrix(
  //      transform_.scale, transform_.rotate, transform_.translate);
  //  modelSphere_.SetWorld(worldSphere);
  //  modelSphere_.SetLightingMode(lightingMode_);
  //  modelSphere_.SetSpecularColor({1.0f, 1.0f, 1.0f});
  //  modelSphere_.SetShininess(64.0f);

  //  // 反射の有効/無効と強さを設定
  //  float finalCoeff = enableReflection_ ? reflectionWeight_ : 0.0f;
  //  modelSphere_.SetEnvironmentCoefficient(finalCoeff);

  //  modelSphere_.Draw();
  //}

  // Skybox
  {
    Matrix4x4 viewMatrix = renderer->GetViewMatrix();
    Matrix4x4 projMatrix = renderer->GetProjectionMatrix();
    Matrix4x4 invView = Inverse(viewMatrix);
    Vector3 camPos = {invView.m[3][0], invView.m[3][1], invView.m[3][2]};

    skybox_.Update(viewMatrix, projMatrix, camPos, {100.0f, 100.0f, 100.0f});
    skybox_.Draw();
  }

  // ヒットエフェクトの描画
  for (auto &ef : hitEffects_) {
    Renderer::GetInstance()->DrawEffectModel(&ef.instance);
  }

  // Ring の描画
  /*{
    Matrix4x4 worldRing = MakeAffineMatrix(ringTransform_.scale, ringTransform_.rotate, ringTransform_.translate);
    ring_.SetTransform(worldRing, camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
    
    Matrix4x4 uvTransform = MakeScaleMatrix({ ringUVScale_.x, ringUVScale_.y, 1.0f });
    ring_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f}, uvTransform);
    if (texRing_) {
        Renderer::GetInstance()->DrawRing(&ring_, texRing_->GetSrvGpu());
    }
  }*/

  // Cylinder の描画
  /*{
    Matrix4x4 worldCylinder = MakeAffineMatrix(cylinderTransform_.scale, cylinderTransform_.rotate, cylinderTransform_.translate);
    cylinder_.SetTransform(worldCylinder, camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
    
    Matrix4x4 uvTransform = MakeScaleMatrix({ cylinderUVScale_.x, cylinderUVScale_.y, 1.0f });
    cylinder_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f}, uvTransform);
    if (texCylinder_) {
        Renderer::GetInstance()->DrawCylinder(&cylinder_, texCylinder_->GetSrvGpu());
    }
  }*/

  // パーティクルの描画
  /*BlendMode pMode = BlendMode::Alpha;
  switch (particleBlendMode_) {
  case 0: pMode = BlendMode::Alpha; break;
  case 1: pMode = BlendMode::Add; break;
  case 2: pMode = BlendMode::Subtract; break;
  case 3: pMode = BlendMode::Multiply; break;
  case 4: pMode = BlendMode::Screen; break;
  }
  ParticleManager::GetInstance()->Draw(pMode);*/

  // 最後にPrimitive（グリッド等）
  Renderer::GetInstance()->RenderPrimitives();

  //    sprite_.Draw();

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

  // レベルオブジェクトの描画
  for (auto& obj : levelObjects_) {
      obj->Draw({ 0.5f, 1.0f, 1.0f, 1.0f }); // コライダーは水色
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
  resCube_ = ModelManager::GetInstance()->Load("resources/cube/cube.obj");
  resEffect_ =
      ModelManager::GetInstance()->Load("resources/particle/particle.obj");

  CheckFileExists_("resources/particle/circle.png");
  CheckFileExists_("resources/sound/select.mp3");

  {
    ModelInstance::CreateInfo ci{};
    ci.resource = resSphere_;
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 0;
    CheckBoolOrDie_(modelSphere_.Initialize(ci), "modelSphere_.Initialize");
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
    ringParams_.divide = 32;
    ringParams_.outerRadius = 2.0f;
    ringParams_.innerRadius = 0.5f;
    ringParams_.colorInner = { 1.0f, 1.0f, 1.0f, 1.0f };
    ringParams_.colorOuter = { 1.0f, 1.0f, 1.0f, 1.0f };
    ringParams_.alphaReference = 0.5f; // デフォルトで半分削る
    
    auto* dx = Renderer::GetInstance()->GetDX();
    ring_.Initialize(dx->GetDevice(), ringParams_);
    texRing_ = TextureManager::GetInstance()->Load("resources/gradationLine.png");
    
    ringTransform_.translate = { 0.0f, 2.0f, 0.0f };
    ringUVScale_ = { 10.0f, 1.0f }; // 10回繰り返して柱っぽくする
  }

  {
    cylinderParams_.divide = 32;
    cylinderParams_.topRadiusX = 1.0f;
    cylinderParams_.topRadiusZ = 1.0f;
    cylinderParams_.bottomRadiusX = 1.0f;
    cylinderParams_.bottomRadiusZ = 1.0f;
    cylinderParams_.height = 3.0f;
    cylinderParams_.colorTop = { 1.0f, 1.0f, 1.0f, 1.0f };
    cylinderParams_.colorBottom = { 1.0f, 1.0f, 1.0f, 1.0f };
    cylinderParams_.alphaReference = 0.5f;
    
    auto* dx = Renderer::GetInstance()->GetDX();
    cylinder_.Initialize(dx->GetDevice(), cylinderParams_);
    texCylinder_ = TextureManager::GetInstance()->Load("resources/gradationLine.png");
    
    cylinderTransform_.translate = { -3.0f, 0.0f, 0.0f };
    cylinderUVScale_ = { 10.0f, 1.0f };
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

  const float aspect = Renderer::GetInstance()->GetAspectRatio();

  if (camera_) {
    camera_->SetPerspective(0.45f, aspect, 0.1f, 100.0f);
  }
}

void GameScene::SpawnHitEffect(const Vector3 &pos) {
  // 1. ベースとして particle.obj (Plane) を出す
 /* HitEffect ef;
  ModelInstance::CreateInfo ci{};
  ci.resource = resEffect_;
  ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  ci.lightingMode = 0;

  ef.instance.Initialize(ci);
  ef.position = pos;
  ef.isActive = true;
  hitEffects_.push_back(std::move(ef));*/

  // 2. 放射状の縦長パーティクルを8個バースト発生 (資料3〜4枚目)
  std::mt19937& rng = []() -> std::mt19937& {
      static std::mt19937 engine{ std::random_device{}() };
      return engine;
  }();
  std::uniform_real_distribution<float> distRotate(-3.14159265f, 3.14159265f);
  std::uniform_real_distribution<float> distScale(0.4f, 1.5f);

  Vector3 baseScale = { 0.05f, 1.0f, 1.0f }; // 資料3枚目

  for (int i = 0; i < 8; ++i) {
      float rotZ = distRotate(rng);
      float scaleY = distScale(rng);
      Vector3 finalScale = { baseScale.x, scaleY, baseScale.z };
      Vector3 rotate = { 0.0f, 0.0f, rotZ };
      Vector3 velocity = { 0.0f, 0.0f, 0.0f }; // 動かない
      Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
      float lifetime = 0.5f;

      ParticleManager::GetInstance()->Emit(
          particleGroupName_, pos, velocity, finalScale, rotate, lifetime, color);
    }
}

void GameScene::LevelObject::Update() {
    float toRad = 3.14159265f / 180.0f;
    Vector3 radRot = { rotation.x * toRad, rotation.y * toRad, rotation.z * toRad };
    Matrix4x4 localMatrix = MakeAffineMatrix(scaling, radRot, translation);
    
    if (parent) {
        worldMatrix = Multiply(localMatrix, parent->worldMatrix);
    } else {
        worldMatrix = localMatrix;
    }
    
    if (model) {
        model->SetWorld(worldMatrix);
    }
    
    for (auto& child : children) {
        child->Update();
    }
}

void GameScene::LevelObject::Draw(const Vector4& colliderColor) {
    if (model) {
        model->Draw();
    }
    
    // コライダーの描画
    if (collider) {
        // 8頂点を算出 (ローカル空間)
        Vector3 min = { collider->center.x - collider->size.x * 0.5f, collider->center.y - collider->size.y * 0.5f, collider->center.z - collider->size.z * 0.5f };
        Vector3 max = { collider->center.x + collider->size.x * 0.5f, collider->center.y + collider->size.y * 0.5f, collider->center.z + collider->size.z * 0.5f };
        
        Vector3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };

        // ワールド座標に変換
        Vector3 worldCorners[8];
        for (int i = 0; i < 8; ++i) {
            Vector3 v = corners[i];
            worldCorners[i].x = v.x * worldMatrix.m[0][0] + v.y * worldMatrix.m[1][0] + v.z * worldMatrix.m[2][0] + worldMatrix.m[3][0];
            worldCorners[i].y = v.x * worldMatrix.m[0][1] + v.y * worldMatrix.m[1][1] + v.z * worldMatrix.m[2][1] + worldMatrix.m[3][1];
            worldCorners[i].z = v.x * worldMatrix.m[0][2] + v.y * worldMatrix.m[1][2] + v.z * worldMatrix.m[2][2] + worldMatrix.m[3][2];
        }

        auto* renderer = Renderer::GetInstance();
        // 前面
        renderer->DrawLine(worldCorners[0], worldCorners[1], colliderColor);
        renderer->DrawLine(worldCorners[2], worldCorners[3], colliderColor);
        renderer->DrawLine(worldCorners[0], worldCorners[2], colliderColor);
        renderer->DrawLine(worldCorners[1], worldCorners[3], colliderColor);
        // 奥面
        renderer->DrawLine(worldCorners[4], worldCorners[5], colliderColor);
        renderer->DrawLine(worldCorners[6], worldCorners[7], colliderColor);
        renderer->DrawLine(worldCorners[4], worldCorners[6], colliderColor);
        renderer->DrawLine(worldCorners[5], worldCorners[7], colliderColor);
        // 接続
        renderer->DrawLine(worldCorners[0], worldCorners[4], colliderColor);
        renderer->DrawLine(worldCorners[1], worldCorners[5], colliderColor);
        renderer->DrawLine(worldCorners[2], worldCorners[6], colliderColor);
        renderer->DrawLine(worldCorners[3], worldCorners[7], colliderColor);
    }
    
    for (auto& child : children) {
        child->Draw(colliderColor);
    }
}

void GameScene::LoadLevel_(const std::string& name) {
    levelData_ = LevelLoader::Load(name);
    if (!levelData_) return;
    
    levelObjects_.clear();
    for (auto& objData : levelData_->objects) {
        CreateLevelObjectRecursive_(objData, nullptr, &levelObjects_);
    }
}

void GameScene::CreateLevelObjectRecursive_(const LevelData::ObjectData& data, LevelObject* parent, std::vector<std::unique_ptr<LevelObject>>* list) {
    auto newObj = std::make_unique<LevelObject>();
    newObj->name = data.name;
    newObj->translation = data.translation;
    newObj->rotation = data.rotation;
    newObj->scaling = data.scaling;
    newObj->parent = parent;
    newObj->collider = data.collider;

    if (data.type == "MESH" && data.fileName) {
        // "resources/" + fileName で読み込む
        // ※現状、Blenderのメッシュ名とフォルダ名が一致している必要がある
        // (例: blenderでbox -> resources/box/box.obj)
        auto resource = ModelManager::GetInstance()->Load("resources/" + *data.fileName + "/" + *data.fileName + ".obj");
        if (!resource) {
            // 失敗した場合は直接指定も試行
            resource = ModelManager::GetInstance()->Load("resources/" + *data.fileName);
        }
        
        if (resource) {
            newObj->model = std::make_unique<ModelInstance>();
            ModelInstance::CreateInfo ci{};
            ci.resource = resource;
            newObj->model->Initialize(ci);
        } else {
            OutputDebugStringA(("Failed to load model: " + *data.fileName + "\n").c_str());
        }
    } else if (data.type == "CAMERA") {
        // TODO: カメラの設定を反映
        OutputDebugStringA(("Found Camera: " + data.name + " (Applying transform to game camera would go here)\n").c_str());
    } else if (data.type == "LIGHT") {
        // TODO: ライトの設定を反映
        OutputDebugStringA(("Found Light: " + data.name + "\n").c_str());
    }

    for (auto& childData : data.children) {
        CreateLevelObjectRecursive_(childData, newObj.get(), &newObj->children);
    }

    list->push_back(std::move(newObj));
}