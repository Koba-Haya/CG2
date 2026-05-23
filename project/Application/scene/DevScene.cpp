#define NOMINMAX
#include "DevScene.h"
#include "SceneIds.h"
#include "SceneManager.h"
#include "DebugCamera.h"
#include "AbsoluteEngine/editor/EditorCamera.h"
#include "DirectXCommon.h"
#include "Renderer.h"
#include "TextureResource.h"
#include "graphics/texture/TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#ifdef USE_IMGUI
#include <imgui.h>
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

DevScene::DevScene() {}
DevScene::~DevScene() {}

void DevScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  InitLogging_();
  InitResources_();

  camera_ = std::make_unique<AbsoluteEngine::EditorCamera>();
  camera_->Initialize();

  InitCamera_();

  accelerationField_.area.max = {1.0f, 1.0f, 1.0f};

  // --- エディタUIの初期化とテストオブジェクト追加 ---
  editorUIManager_ = std::make_unique<AbsoluteEngine::EditorUIManager>();
  
  auto obj1 = std::make_shared<AbsoluteEngine::GameObject>("Player");
  obj1->GetTransform().translate = { 0.0f, 0.0f, 0.0f };
  
  auto obj2 = std::make_shared<AbsoluteEngine::GameObject>("Enemy");
  obj2->GetTransform().translate = { 5.0f, 0.0f, 0.0f };
  
  auto obj3 = std::make_shared<AbsoluteEngine::GameObject>("Weapon");
  obj3->GetTransform().translate = { 1.0f, 0.0f, 0.0f };
  obj1->AddChild(obj3); // Playerの子にする
  
  rootObjects_.push_back(obj1);
  rootObjects_.push_back(obj2);
}

void DevScene::Finalize() {
  if (logStream_.is_open()) {
    logStream_.flush();
    logStream_.close();
  }
}

void DevScene::Update() {
  const float deltaTime = 1.0f / 60.0f;
  
  if (camera_) {
    camera_->Update(*services_.input);
  }

#ifdef USE_IMGUI
  // --- ゲームビューポートウィンドウ ---
  // オフスクリーンレンダリングの結果を ImGui ウィンドウ内に表示する
  // ウィンドウをドッキングして中央に配置することでゲーム画面になる
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Viewport##GameView");
  ImGui::PopStyleVar();
  if (renderTexture_) {
    // ウィンドウのコンテンツ領域サイズに合わせてゲーム画面をリサイズ表示
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
    if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = renderTexture_->GetSrvGpuHandle();
    ImGui::Image(static_cast<ImTextureID>(srvHandle.ptr), viewportSize);

    // ビューポートへのドロップ（モデル生成）
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MODEL_PATH")) {
        const char* payloadPath = (const char*)payload->Data;
        auto newObj = std::make_shared<AbsoluteEngine::GameObject>("Model");
        newObj->LoadModel(payloadPath);
        
        // カメラの前方などに配置するのが理想ですが、今回は原点配置
        newObj->GetTransform().translate = {0, 0, 0};
        
        rootObjects_.push_back(newObj);
        
        if (editorUIManager_) {
          editorUIManager_->SetSelectedObject(newObj);
        }
      }
      ImGui::EndDragDropTarget();
    }
  }
  ImGui::End();

  // --- 左パネル：ライト設定 ---
  ImGui::Begin("Lights##LeftPanel");
  ImGui::Separator();

  if (ImGui::CollapsingHeader("DirectionalLights")) {
    ImGui::Checkbox("Enable DirectionalLights", &enableDirectionalLight_);
    if (!dirLights_.empty()) {
      DirLight &dl = dirLights_[0];
      ImGui::ColorEdit3("DirColor", &dl.color.x);
      ImGui::DragFloat3("DirDirection", &dl.direction.x, 0.01f, -1.0f, 1.0f);
      ImGui::SliderFloat("DirIntensity", &dl.intensity, 0.0f, 10.0f);
    }
  }

  if (ImGui::CollapsingHeader("PointLights")) {
    ImGui::Checkbox("Enable PointLights", &enablePointLight_);
    if (!pointLights_.empty()) {
      PointLight &pl = pointLights_[0];
      ImGui::ColorEdit3("PointColor", &pl.color.x);
      ImGui::DragFloat3("PointPosition", &pl.position.x, 0.1f);
      ImGui::SliderFloat("PointIntensity", &pl.intensity, 0.0f, 10.0f);
    }
  }

  if (ImGui::CollapsingHeader("SpotLights")) {
    ImGui::Checkbox("Enable SpotLights", &enableSpotLight_);
    if (!spotLights_.empty()) {
      SpotLight &sl = spotLights_[0];
      ImGui::ColorEdit3("SpotColor", &sl.color.x);
      ImGui::DragFloat3("SpotPosition", &sl.position.x, 0.1f);
      ImGui::SliderFloat("SpotIntensity", &sl.intensity, 0.0f, 10.0f);
    }
  }
  ImGui::End();

  // --- 右パネル：プリミティブ設定 ---
  ImGui::Begin("Primitives##RightPanel");
  if (ImGui::CollapsingHeader("Ring Primitive")) {
    bool changed = false;
    int divide = static_cast<int>(ringParams_.divide);
    if (ImGui::SliderInt("Divide##Ring", &divide, 3, 128)) {
      ringParams_.divide = static_cast<uint32_t>(divide);
      changed = true;
    }
    changed |= ImGui::SliderFloat("Outer Radius", &ringParams_.outerRadius, 0.1f, 10.0f);
    changed |= ImGui::SliderFloat("Inner Radius", &ringParams_.innerRadius, 0.0f, 10.0f);
    changed |= ImGui::SliderAngle("Start Angle", &ringParams_.startAngle);
    changed |= ImGui::SliderAngle("End Angle", &ringParams_.endAngle);
    changed |= ImGui::ColorEdit4("Color Inner", &ringParams_.colorInner.x);
    changed |= ImGui::ColorEdit4("Color Outer", &ringParams_.colorOuter.x);
    if (changed) {
      ring_.Update(Renderer::GetInstance()->GetDX()->GetDevice(), ringParams_);
    }
    ImGui::DragFloat3("Ring Pos", &ringTransform_.translate.x, 0.1f);
    ImGui::DragFloat3("Ring Rot", &ringTransform_.rotate.x, 0.05f);
  }

  if (ImGui::CollapsingHeader("Cylinder Primitive")) {
    bool changed = false;
    int divide = static_cast<int>(cylinderParams_.divide);
    if (ImGui::SliderInt("Divide##Cyl", &divide, 3, 128)) {
      cylinderParams_.divide = static_cast<uint32_t>(divide);
      changed = true;
    }
    changed |= ImGui::DragFloat2("Top Radius (X,Z)", &cylinderParams_.topRadiusX, 0.1f);
    changed |= ImGui::DragFloat2("Bottom Radius (X,Z)", &cylinderParams_.bottomRadiusX, 0.1f);
    changed |= ImGui::SliderFloat("Height##Cyl", &cylinderParams_.height, 0.1f, 10.0f);
    if (changed) {
      cylinder_.Update(Renderer::GetInstance()->GetDX()->GetDevice(), cylinderParams_);
    }
    ImGui::DragFloat3("Cylinder Pos", &cylinderTransform_.translate.x, 0.1f);
    ImGui::DragFloat3("Cylinder Rot", &cylinderTransform_.rotate.x, 0.05f);
  }
  ImGui::End();

  // --- 下パネル：オブジェクト・エフェクト設定 ---
  ImGui::Begin("Objects##BottomPanel");
  ImGui::Checkbox("Show Skeleton", &showSkeleton_);
  ImGui::Checkbox("Enable Reflection", &enableReflection_);
  if (enableReflection_) {
    ImGui::SliderFloat("Reflection Weight", &reflectionWeight_, 0.0f, 1.0f);
  }

  ImGui::SeparatorText("PostProcess");
  int mode = static_cast<int>(postProcessMode_);
  if (ImGui::RadioButton("Normal", &mode, static_cast<int>(Renderer::PostProcessMode::Normal))) postProcessMode_ = Renderer::PostProcessMode::Normal;
  ImGui::SameLine();
  if (ImGui::RadioButton("Grayscale", &mode, static_cast<int>(Renderer::PostProcessMode::Grayscale))) postProcessMode_ = Renderer::PostProcessMode::Grayscale;
  ImGui::SameLine();
  if (ImGui::RadioButton("Sepia", &mode, static_cast<int>(Renderer::PostProcessMode::Sepia))) postProcessMode_ = Renderer::PostProcessMode::Sepia;

  const char *blendModeItems[] = {"Alpha", "Add", "Subtract", "Multiply", "Screen"};
  ImGui::Combo("Particle Blend", &particleBlendMode_, blendModeItems, IM_ARRAYSIZE(blendModeItems));

  ImGui::SeparatorText("Transform");
  ImGui::DragFloat3("Sphere Pos", &transform_.translate.x, 0.1f);
  ImGui::DragFloat3("Human Pos", &transformHuman_.translate.x, 0.1f);
  auto* editorCamera = dynamic_cast<AbsoluteEngine::EditorCamera*>(camera_.get());
  if (editorCamera) {
      Vector3 camPos = editorCamera->GetTranslate();
      if (ImGui::DragFloat3("Camera Pos", &camPos.x, 0.1f)) {
          editorCamera->SetTranslate(camPos);
      }
      Vector3 camRot = editorCamera->GetRotation();
      if (ImGui::DragFloat3("Camera Rot(Pitch,Yaw,Roll)", &camRot.x, 0.05f)) {
          editorCamera->SetRotation(camRot);
      }
  } else {
      ImGui::DragFloat3("Camera Pos (Not Linked)", &cameraTransform_.translate.x, 0.1f);
  }
  ImGui::End();

  // --- エディタUIの描画（Hierarchy, Inspector, Gizmo） ---
  if (editorUIManager_ && camera_) {
    editorUIManager_->DrawUI(rootObjects_, camera_->GetViewMatrix(), camera_->GetProjectionMatrix(), dynamic_cast<AbsoluteEngine::EditorCamera*>(camera_.get()));
  }

#endif

  ringTransform_.rotate.z += 1.5f * deltaTime;
  cylinderTransform_.rotate.y += 1.0f * deltaTime;

  particleEmitter_.Update(deltaTime);
  ParticleManager::GetInstance()->SetEnableAccelerationField(enableAccelerationField_);
  ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);
  ParticleManager::GetInstance()->Update(deltaTime);

  modelAnimCube_.UpdateAnimation(deltaTime);
  modelSimpleSkin_.UpdateAnimation(deltaTime);
  modelHuman_.UpdateAnimation(deltaTime);

  if (services_.input->TriggerKey(DIK_SPACE)) {
    SpawnHitEffect({0.0f, 0.0f, -1.0f});
  }

  // エフェクト更新
  for (auto &ef : hitEffects_) {
    if (!ef.isActive) continue;
    ef.frame += 1.0f;
    float t = ef.frame / ef.maxFrame;
    float scaleVal = t * 5.0f;
    ef.instance.SetWorld(MakeAffineMatrix(Vector3{scaleVal, scaleVal, scaleVal}, Vector3{0, 0, 0}, ef.position));
    ef.instance.SetColor({1.0f, 1.0f, 1.0f, 1.0f - t});
    if (ef.frame >= ef.maxFrame) ef.isActive = false;
  }
  hitEffects_.erase(std::remove_if(hitEffects_.begin(), hitEffects_.end(), [](const HitEffect &e) { return !e.isActive; }), hitEffects_.end());
}

void DevScene::Draw() {
  auto* renderer = Renderer::GetInstance();
  auto* dx = renderer->GetDX();
  auto* cmdList = dx->GetCommandList();

  // --- カメラ・ライト設定（オフスクリーンパス前に確定させる） ---
  if (camera_) {
    renderer->SetCamera(*camera_);
  }
  renderer->SetEnvironmentMap(skybox_.GetTexture());
  renderer->SetDirectionalLights(dirLights_, enableDirectionalLight_);
  renderer->SetPointLights(pointLights_, enablePointLight_);
  renderer->SetSpotLights(spotLights_, enableSpotLight_);

  // --- オフスクリーン描画パス（RenderTexture → ImGui::Image で表示） ---
  if (renderTexture_) {
    // RenderTexture に切り替え（SRV → RT バリア + OMSetRenderTargets）
    dx->SetRenderTarget(renderTexture_.get());

    // RenderTexture をクリア（暗いグレー）
    float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    cmdList->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);

    // 深度バッファをクリア
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        GetCPUDescriptorHandle(dx->GetDSVHeap(), dx->GetDSVDescriptorSize(), 0);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // --- シーンを RenderTexture に描画 ---
    modelSphere_.SetWorld(MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate));
    modelSphere_.SetEnvironmentCoefficient(enableReflection_ ? reflectionWeight_ : 0.0f);
    modelSphere_.Draw();

    modelAnimCube_.SetWorld(MakeAffineMatrix(transformAnimCube_.scale, transformAnimCube_.rotate, transformAnimCube_.translate));
    modelAnimCube_.Draw();

    modelSimpleSkin_.Draw();
    modelHuman_.Draw();

    if (showSkeleton_) {
      modelSimpleSkin_.DrawSkeleton();
      modelHuman_.DrawSkeleton();
      modelAnimCube_.DrawSkeleton();
    }

    // --- エディタ上で配置したオブジェクト群の描画 ---
    for (auto& obj : rootObjects_) {
      obj->Draw();
    }

    renderer->RenderPrimitives();
    skybox_.Draw();

    // Ring & Cylinder
    {
      ring_.SetTransform(
          MakeAffineMatrix(ringTransform_.scale, ringTransform_.rotate, ringTransform_.translate),
          camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
      ring_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f},
          MakeScaleMatrix({ringUVScale_.x, ringUVScale_.y, 1.0f}));
      if (texRing_) renderer->DrawRing(&ring_, texRing_->GetSrvGpu());

      cylinder_.SetTransform(
          MakeAffineMatrix(cylinderTransform_.scale, cylinderTransform_.rotate, cylinderTransform_.translate),
          camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
      cylinder_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f},
          MakeScaleMatrix({cylinderUVScale_.x, cylinderUVScale_.y, 1.0f}));
      if (texCylinder_) renderer->DrawCylinder(&cylinder_, texCylinder_->GetSrvGpu());
    }

    for (auto& ef : hitEffects_) renderer->DrawEffectModel(&ef.instance);

    // GPUパーティクル描画
    renderer->DrawGPUParticles(static_cast<BlendMode>(particleBlendMode_ + 1));

    // RenderTexture を SRV 状態に戻し、バックバッファに切り替え
    dx->FinishRendering(renderTexture_.get());
  }

  // --- バックバッファへの追加描画（必要なら UI 等をここに） ---
  // 現在はImGuiがメインのUIとなるため、バックバッファへの直接描画はなし
}

void DevScene::InitLogging_() {
  std::filesystem::create_directory("logs");
  logStream_.open("logs/dev_scene.log"); // シンプル化
}

void DevScene::InitResources_() {
  auto *mm = ModelManager::GetInstance();
  auto *tm = TextureManager::GetInstance();
  auto *dx = Renderer::GetInstance()->GetDX();

  resSphere_ = mm->Load("resources/app/sphere/sphere.obj");
  resTerrain_ = mm->Load("resources/app/terrain/terrain.obj");
  resCube_ = mm->Load("resources/app/cube/cube.obj");
  resAnimCube_ = mm->Load("resources/app/AnimatedCube/AnimatedCube.gltf");
  animCubeAnim_ = AnimationManager::GetInstance()->LoadAnimation("resources/app/AnimatedCube", "AnimatedCube.gltf");
  resEffect_ = mm->Load("resources/app/particle/particle.obj");

  modelSphere_.Initialize({resSphere_, {1, 1, 1, 1}, 0});
    modelTerrain_.Initialize({resTerrain_, {1, 1, 1, 1}, 0});
  modelAnimCube_.Initialize({resAnimCube_, {1, 1, 1, 1}, 1});
  if (animCubeAnim_) modelAnimCube_.PlayAnimation(animCubeAnim_, true);

  resSimpleSkin_ = mm->Load("resources/app/simpleSkin/simpleSkin.gltf");
  animSimpleSkin_ = AnimationManager::GetInstance()->LoadAnimation("resources/app/simpleSkin", "simpleSkin.gltf");
  modelSimpleSkin_.Initialize({resSimpleSkin_, {1, 1, 1, 1}, 1});
  if (animSimpleSkin_) modelSimpleSkin_.PlayAnimation(animSimpleSkin_, true);

  resHuman_ = mm->Load("resources/app/human/walk.gltf");
  animHuman_ = AnimationManager::GetInstance()->LoadAnimation("resources/app/human", "walk.gltf");
  modelHuman_.Initialize({resHuman_, {1, 1, 1, 1}, 1});
  if (animHuman_) modelHuman_.PlayAnimation(animHuman_, true);

  sprite_.Initialize({"resources/app/plane/uvChecker.png", {640, 360}, {1, 1, 1, 1}});
  skybox_.Initialize("resources/app/dds/dds.dds");

  ringParams_.divide = 32;
  ringParams_.outerRadius = 2.0f;
  ringParams_.innerRadius = 1.8f;
  ring_.Initialize(dx->GetDevice(), ringParams_);
  texRing_ = tm->Load("resources/app/textures/gradationLine.png");
  ringTransform_.translate = {0.0f, 2.0f, 0.0f};
  ringUVScale_ = {10.0f, 1.0f};

  cylinderParams_.divide = 32;
  cylinderParams_.topRadiusX = 0.5f;
  cylinderParams_.bottomRadiusX = 0.5f;
  cylinderParams_.height = 4.0f;
  cylinder_.Initialize(dx->GetDevice(), cylinderParams_);
  texCylinder_ = tm->Load("resources/app/textures/gradationLine.png");
  cylinderTransform_.translate = {-4.0f, 0.0f, 0.0f};
  cylinderUVScale_ = {5.0f, 1.0f};

  ParticleManager::GetInstance()->CreateParticleGroup(particleGroupName_, "resources/app/particle/circle.png", kParticleCount_);
  ParticleEmitter::Params p{};
  p.groupName = particleGroupName_;
  p.emitRate = 10.0f;
  particleEmitter_.Initialize(ParticleManager::GetInstance(), p);

  dirLights_.push_back({{1,1,1}, {0,-1,0}, 1.0f});
  pointLights_.push_back({{1,1,1}, {0,2,-2}, 1.0f, 10.0f, 2.0f});

  // オフスクリーンテスト初期化
  renderTexture_ = std::make_unique<RenderTexture>();
  renderTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {1,0,0,1});
}

void DevScene::InitCamera_() {
  transformTerrain_.translate = {0, -5.0f, 0};
  cameraTransform_.translate = {0, 0, -15};
  transformSimpleSkin_.translate = {3, 0, 0};
  transformHuman_.translate = {6, 0, 0};
  transformAnimCube_.translate = {-3, 0, 0};
  if (camera_) camera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);
}

void DevScene::SpawnHitEffect(const Vector3 &pos) {
  for (int i = 0; i < 8; ++i) {
    ParticleManager::GetInstance()->Emit(
        particleGroupName_, pos, Vector3{0, 0, 0}, Vector3{0.05f, 1.0f, 1.0f},
        Vector3{0, 0, (float)i}, 0.5f, Vector4{1, 1, 1, 1});
  }
}
