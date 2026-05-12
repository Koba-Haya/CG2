#define NOMINMAX
#include "GameScene.h"
#include "DebugCamera.h"
#include "DirectXCommon.h"
#include "Renderer.h"
#include "TextureResource.h"
#include "graphics/texture/TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"

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

  const float deltaTime = 1.0f / 60.0f;

#ifdef USE_IMGUI
  static bool settingsOpen = true;

  // --- ウィンドウ1: Settings ---
  ImGui::Begin("Settings", &settingsOpen);
  {
    // DirectionalLights
    if (ImGui::CollapsingHeader("DirectionalLights")) {
      ImGui::Checkbox("Enable DirectionalLights", &enableDirectionalLight_);
      if (!dirLights_.empty()) {
        DirLight &dl = dirLights_[0];
        ImGui::ColorEdit3("DirColor", &dl.color.x);
        ImGui::DragFloat3("DirDirection", &dl.direction.x, 0.01f, -1.0f, 1.0f);
        ImGui::SliderFloat("DirIntensity", &dl.intensity, 0.0f, 10.0f);
      }
    }

    // PointLights
    if (ImGui::CollapsingHeader("PointLights")) {
      ImGui::Checkbox("Enable PointLights", &enablePointLight_);
      if (!pointLights_.empty()) {
        PointLight &pl = pointLights_[0];
        ImGui::ColorEdit3("PointColor", &pl.color.x);
        ImGui::DragFloat3("PointPosition", &pl.position.x, 0.1f);
        ImGui::SliderFloat("PointIntensity", &pl.intensity, 0.0f, 10.0f);
      }
    }

    // SpotLights
    if (ImGui::CollapsingHeader("SpotLights")) {
      ImGui::Checkbox("Enable SpotLights", &enableSpotLight_);
      if (!spotLights_.empty()) {
        SpotLight &sl = spotLights_[0];
        ImGui::ColorEdit3("SpotColor", &sl.color.x);
        ImGui::DragFloat3("SpotPosition", &sl.position.x, 0.1f);
        ImGui::SliderFloat("SpotIntensity", &sl.intensity, 0.0f, 10.0f);
      }
    }

    // Ring Primitive 調整
    if (ImGui::CollapsingHeader("Ring Primitive")) {
      bool changed = false;
      int divide = static_cast<int>(ringParams_.divide);
      if (ImGui::SliderInt("Divide##Ring", &divide, 3, 128)) {
        ringParams_.divide = static_cast<uint32_t>(divide);
        changed = true;
      }
      changed |= ImGui::SliderFloat("Outer Radius", &ringParams_.outerRadius,
                                    0.1f, 10.0f);
      changed |= ImGui::SliderFloat("Inner Radius", &ringParams_.innerRadius,
                                    0.0f, 10.0f);
      changed |= ImGui::SliderAngle("Start Angle", &ringParams_.startAngle);
      changed |= ImGui::SliderAngle("End Angle", &ringParams_.endAngle);
      changed |= ImGui::Checkbox("UV Vertical##Ring", &ringParams_.uvVertical);
      changed |= ImGui::ColorEdit4("Color Inner", &ringParams_.colorInner.x);
      changed |= ImGui::ColorEdit4("Color Outer", &ringParams_.colorOuter.x);
      changed |= ImGui::SliderFloat("Alpha Ref##Ring",
                                    &ringParams_.alphaReference, 0.0f, 1.0f);
      ImGui::DragFloat2("UV Scale##Ring", &ringUVScale_.x, 0.1f);

      if (changed) {
        ring_.Update(Renderer::GetInstance()->GetDX()->GetDevice(),
                     ringParams_);
      }
      ImGui::DragFloat3("Ring Pos", &ringTransform_.translate.x, 0.1f);
      ImGui::DragFloat3("Ring Rot", &ringTransform_.rotate.x, 0.05f);
    }

    // Cylinder Primitive 調整
    if (ImGui::CollapsingHeader("Cylinder Primitive")) {
      bool changed = false;
      int divide = static_cast<int>(cylinderParams_.divide);
      if (ImGui::SliderInt("Divide##Cyl", &divide, 3, 128)) {
        cylinderParams_.divide = static_cast<uint32_t>(divide);
        changed = true;
      }
      changed |= ImGui::DragFloat2("Top Radius (X,Z)",
                                   &cylinderParams_.topRadiusX, 0.1f);
      changed |= ImGui::DragFloat2("Bottom Radius (X,Z)",
                                   &cylinderParams_.bottomRadiusX, 0.1f);
      changed |= ImGui::SliderFloat("Height##Cyl", &cylinderParams_.height,
                                    0.1f, 10.0f);
      changed |= ImGui::Checkbox("Flip V##Cyl", &cylinderParams_.flipV);
      changed |= ImGui::ColorEdit4("Color Top", &cylinderParams_.colorTop.x);
      changed |=
          ImGui::ColorEdit4("Color Bottom", &cylinderParams_.colorBottom.x);
      ImGui::DragFloat2("UV Scale##Cyl", &cylinderUVScale_.x, 0.1f);

      if (changed) {
        cylinder_.Update(Renderer::GetInstance()->GetDX()->GetDevice(),
                         cylinderParams_);
      }
      ImGui::DragFloat3("Cylinder Pos", &cylinderTransform_.translate.x, 0.1f);
      ImGui::DragFloat3("Cylinder Rot", &cylinderTransform_.rotate.x, 0.05f);
    }
  }
  ImGui::End(); // ウィンドウ1 終了

  // --- ウィンドウ2: Object ---
  ImGui::Begin("Object", &settingsOpen);
  {
    ImGui::Checkbox("Show Skeleton", &showSkeleton_);
    ImGui::Checkbox("Enable Reflection", &enableReflection_);
    if (enableReflection_) {
      ImGui::SliderFloat("Weight", &reflectionWeight_, 0.0f, 1.0f);
    }

    const char *blendModeItems[] = {"Alpha", "Add", "Subtract", "Multiply",
                                    "Screen"};
    ImGui::Combo("Particle Blend", &particleBlendMode_, blendModeItems,
                 IM_ARRAYSIZE(blendModeItems));

    ImGui::SeparatorText("Camera");
    ImGui::DragFloat3("CameraTranslate", &cameraTransform_.translate.x, 0.1f);
    ImGui::DragFloat3("CameraRotate", &cameraTransform_.rotate.x, 0.01f);

    ImGui::SeparatorText("Sphere");
    ImGui::DragFloat3("SphereTranslate", &transform_.translate.x, 0.1f);
    ImGui::DragFloat3("SphereScale", &transform_.scale.x, 0.1f);
  }
  ImGui::End(); // ウィンドウ2 終了

#endif
  ringTransform_.rotate.z += 1.5f * deltaTime;
  cylinderTransform_.rotate.y += 1.0f * deltaTime;

  particleEmitter_.Update(deltaTime);

  ParticleManager::GetInstance()->SetEnableAccelerationField(
      enableAccelerationField_);
  ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);
  ParticleManager::GetInstance()->Update(deltaTime);

  modelAnimCube_.UpdateAnimation(deltaTime);
  modelSimpleSkin_.UpdateAnimation(deltaTime);
  modelHuman_.UpdateAnimation(deltaTime);

  // テスト用：スペースキーで原点にエフェクト発生
  if (services_.input->TriggerKey(DIK_SPACE)) {
    SpawnHitEffect({0.0f, 0.0f, -1.0f});
  }

  // エフェクトの更新
  for (auto &ef : hitEffects_) {
    if (!ef.isActive) {
      continue;
    }

    ef.frame += 1.0f;
    float t = ef.frame / ef.maxFrame; // 0.0 ~ 1.0

    // スケール：時間とともに大きく
    float scaleVal = t * 5.0f;
    Matrix4x4 world = MakeAffineMatrix(Vector3{scaleVal, scaleVal, scaleVal},
                                       Vector3{0, 0, 0}, ef.position);
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

  // --- 不透明描画 ---
  {
    modelSphere_.SetWorld(MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate));
    modelSphere_.SetEnvironmentCoefficient(enableReflection_ ? reflectionWeight_
                                                             : 0.0f);
    modelSphere_.Draw();
  }
  modelAnimCube_.SetWorld(MakeAffineMatrix(transformAnimCube_.scale,
                                           transformAnimCube_.rotate,
                                           transformAnimCube_.translate));
  modelAnimCube_.Draw();

  modelSimpleSkin_.Draw();
  modelHuman_.Draw();

  if (showSkeleton_) {
    modelSimpleSkin_.DrawSkeleton();
    modelHuman_.DrawSkeleton();
    modelAnimCube_.DrawSkeleton();
  }

  renderer->RenderPrimitives();
  skybox_.Draw();

  // --- 透過・加算描画（Ring & Cylinder） ---

  // Ring 描画
  {
    Matrix4x4 worldRing = MakeAffineMatrix(
        ringTransform_.scale, ringTransform_.rotate, ringTransform_.translate);
    ring_.SetTransform(worldRing, camera_->GetViewMatrix(),
                       camera_->GetProjectionMatrix());

    Matrix4x4 uvTransform =
        MakeScaleMatrix({ringUVScale_.x, ringUVScale_.y, 1.0f});
    ring_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f}, uvTransform);

    if (texRing_) {
      renderer->DrawRing(&ring_, texRing_->GetSrvGpu());
    }
  }

  // Cylinder 描画
  {
    Matrix4x4 worldCylinder =
        MakeAffineMatrix(cylinderTransform_.scale, cylinderTransform_.rotate,
                         cylinderTransform_.translate);
    cylinder_.SetTransform(worldCylinder, camera_->GetViewMatrix(),
                           camera_->GetProjectionMatrix());

    Matrix4x4 uvTransform =
        MakeScaleMatrix({cylinderUVScale_.x, cylinderUVScale_.y, 1.0f});
    cylinder_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f}, uvTransform);

    if (texCylinder_) {
      renderer->DrawCylinder(&cylinder_, texCylinder_->GetSrvGpu());
    }
  }

  // エフェクト・パーティクル
  for (auto &ef : hitEffects_) {
    renderer->DrawEffectModel(&ef.instance);
  }

  // パーティクルの描画
  BlendMode pMode = BlendMode::Alpha;
  switch (particleBlendMode_) {
  case 0: pMode = BlendMode::Alpha; break;
  case 1: pMode = BlendMode::Add; break;
  case 2: pMode = BlendMode::Subtract; break;
  case 3: pMode = BlendMode::Multiply; break;
  case 4: pMode = BlendMode::Screen; break;
  }
  ParticleManager::GetInstance()->Draw(pMode);

  // 最後にPrimitive（グリッド等）
  //Renderer::GetInstance()->RenderPrimitives();

  //    sprite_.Draw();

  if (showEmitterGizmo_) {
    const auto &ep = particleEmitter_.GetParams();

    if (ep.shape == EmitterShape::Box) {
      Vector3 scale = {ep.extent.x * 2.0f, ep.extent.y * 2.0f,
                       ep.extent.z * 2.0f};
      Matrix4x4 world =
          MakeAffineMatrix(scale, Vector3{0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterBox_.SetWorld(world);
      modelEmitterBox_.SetWireframe(true);
      modelEmitterBox_.Draw();
    } else {
      Vector3 scale = {std::max(ep.extent.x, 0.001f),
                       std::max(ep.extent.y, 0.001f),
                       std::max(ep.extent.z, 0.001f)};
      Matrix4x4 world =
          MakeAffineMatrix(scale, Vector3{0.0f, 0.0f, 0.0f}, ep.localCenter);
      modelEmitterSphere_.SetWorld(world);
      modelEmitterSphere_.SetWireframe(true);
      modelEmitterSphere_.Draw();
    }
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
  auto *mm = ModelManager::GetInstance();
  auto *tm = TextureManager::GetInstance();
  auto *dx = Renderer::GetInstance()->GetDX();

  resSphere_ = mm->Load("resources/sphere/sphere.obj");
  resCube_ = mm->Load("resources/cube/cube.obj");
  resAnimCube_ = mm->Load("resources/AnimatedCube/AnimatedCube.gltf");
  animCubeAnim_ = AnimationManager::GetInstance()->LoadAnimation(
      "resources/AnimatedCube", "AnimatedCube.gltf");
  resEffect_ = mm->Load("resources/particle/particle.obj");

  modelSphere_.Initialize({resSphere_, {1, 1, 1, 1}, 0});
  modelAnimCube_.Initialize({resAnimCube_, {1, 1, 1, 1}, 1});
  if (animCubeAnim_)
    modelAnimCube_.PlayAnimation(animCubeAnim_, true);

  resSimpleSkin_ = mm->Load("resources/simpleSkin/simpleSkin.gltf");
  animSimpleSkin_ = AnimationManager::GetInstance()->LoadAnimation(
      "resources/simpleSkin", "simpleSkin.gltf");
  modelSimpleSkin_.Initialize({resSimpleSkin_, {1, 1, 1, 1}, 1});
  if (animSimpleSkin_)
    modelSimpleSkin_.PlayAnimation(animSimpleSkin_, true);

  resHuman_ = mm->Load("resources/human/walk.gltf");
  animHuman_ = AnimationManager::GetInstance()->LoadAnimation("resources/human",
                                                              "walk.gltf");
  modelHuman_.Initialize({resHuman_, {1, 1, 1, 1}, 1});
  if (animHuman_)
    modelHuman_.PlayAnimation(animHuman_, true);

  modelEmitterSphere_.Initialize({resSphere_, {0.3f, 0.8f, 1.0f, 0.3f}, 0});
  modelEmitterBox_.Initialize({resCube_, {1.0f, 0.8f, 0.2f, 0.3f}, 0});

  sprite_.Initialize(
      {"resources/plane/uvChecker.png", {640, 360}, {1, 1, 1, 1}});
  skybox_.Initialize("resources/dds/dds.dds");

  // Ring 初期化
  ringParams_.divide = 32;
  ringParams_.outerRadius = 2.0f;
  ringParams_.innerRadius = 1.8f;
  ringParams_.colorInner = {1, 1, 1, 1};
  ringParams_.colorOuter = {1, 1, 1, 1};
  ringParams_.alphaReference = 0.0f;
  ring_.Initialize(dx->GetDevice(), ringParams_);
  texRing_ = tm->Load("resources/gradationLine.png");
  ringTransform_.translate = {0.0f, 2.0f, 0.0f};
  ringUVScale_ = {10.0f, 1.0f};

  // Cylinder 初期化
  cylinderParams_.divide = 32;
  cylinderParams_.topRadiusX = 0.5f;
  cylinderParams_.topRadiusZ = 0.5f;
  cylinderParams_.bottomRadiusX = 0.5f;
  cylinderParams_.bottomRadiusZ = 0.5f;
  cylinderParams_.height = 4.0f;
  cylinderParams_.colorTop = {1, 1, 1, 1};
  cylinderParams_.colorBottom = {1, 1, 1, 1};
  cylinder_.Initialize(dx->GetDevice(), cylinderParams_);
  texCylinder_ = tm->Load("resources/gradationLine.png");
  cylinderTransform_.translate = {-4.0f, 0.0f, 0.0f};
  cylinderUVScale_ = {5.0f, 1.0f};

  ParticleManager::GetInstance()->CreateParticleGroup(
      particleGroupName_, "resources/particle/circle.png", kParticleCount_);
  ParticleEmitter::Params p{};
  p.groupName = particleGroupName_;
  p.shape = EmitterShape::Box;
  p.emitRate = 10.0f;
  particleEmitter_.Initialize(ParticleManager::GetInstance(), p);

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
  transform_.scale = {1, 1, 1};
  cameraTransform_.translate = {0, 0, -15};
  transformSimpleSkin_.translate = {3, 0, 0};
  transformHuman_.translate = {6, 0, 0};
  transformAnimCube_.translate = {-3, 0, 0};
  if (camera_)
    camera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(),
                            0.1f, 1000.0f);
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

  // 2. 放射状の縦長パーティクルを8個バースト発生
  std::mt19937& rng = []() -> std::mt19937& {
      static std::mt19937 engine{ std::random_device{}() };
      return engine;
  }();
  std::uniform_real_distribution<float> distRotate(-3.14159265f, 3.14159265f);
  std::uniform_real_distribution<float> distScale(0.4f, 1.5f);

  Vector3 baseScale = { 0.05f, 1.0f, 1.0f };

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