#define NOMINMAX
#include "DevScene.h"
#include "SceneIds.h"
#include "SceneManager.h"
#include "DebugCamera.h"
#include "AbsoluteEngine/editor/Command.h"
#include "AbsoluteEngine/scene/SceneSerializer.h"
#include "graphics/Renderer.h"
#include "AbsoluteEngine/editor/EditorCamera.h"
#include "AbsoluteEngine/editor/EditorUIManager.h"
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
#include "AbsoluteEngine/scene/ComponentFactory.h"

// テスト用コンポーネント
class SpinComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (owner_) {
            auto& t = owner_->GetTransform();
            t.rotate.y += 2.0f * deltaTime;
        }
    }
    std::string GetTypeName() const override { return "SpinComponent"; }
};

class MoveComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (owner_) {
            auto& t = owner_->GetTransform();
            t.translate.x += std::sin(frame_ * 0.05f) * 0.05f;
            frame_ += 1.0f;
        }
    }
    std::string GetTypeName() const override { return "MoveComponent"; }
private:
    float frame_ = 0.0f;
};

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



  InitCamera_();

  accelerationField_.area.max = {1.0f, 1.0f, 1.0f};

  // --- サンプルコンポーネントの登録 ---
  AbsoluteEngine::ComponentFactory::GetInstance().Register("SpinComponent", []() { return std::make_unique<SpinComponent>(); });
  AbsoluteEngine::ComponentFactory::GetInstance().Register("MoveComponent", []() { return std::make_unique<MoveComponent>(); });

  // --- エディタUIの初期化とテストオブジェクト追加 ---

  
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
  time_ += deltaTime;
  
  // BaseSceneのエディタ機能（カメラ、オブジェクトの更新）
  UpdateEditor();

#ifdef USE_IMGUI
  // --- ゲームビューポートウィンドウ ---
  // オフスクリーンレンダリングの結果を ImGui ウィンドウ内に表示する
  // ウィンドウをドッキングして中央に配置することでゲーム画面になる
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Viewport##GameView");
  ImGui::PopStyleVar();
  if (postProcessTexture_) {
    // ウィンドウのコンテンツ領域サイズに合わせてゲーム画面をリサイズ表示
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
    if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = postProcessTexture_->GetSrvGpuHandle();
    ImGui::Image(static_cast<ImTextureID>(srvHandle.ptr), viewportSize);
    
    // Viewportへのドラッグ＆ドロップ受付（画像へのドロップ）
    if (editorUIManager_) {
        editorUIManager_->HandleViewportDragDrop(rootObjects_);
    }
  }

  ImGui::End();

  // --- 左パネル：ライト設定（UIから削除し、インスペクター側で管理） ---

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
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Separator();
  ImGui::Checkbox("Show Skeleton", &showSkeleton_);
  ImGui::Checkbox("Enable Reflection", &enableReflection_);
  if (enableReflection_) {
    ImGui::SliderFloat("Reflection Weight", &reflectionWeight_, 0.0f, 1.0f);
  }

  const char *blendModeItems[] = {"Alpha", "Add", "Subtract", "Multiply", "Screen"};
  ImGui::Combo("Particle Blend", &particleBlendMode_, blendModeItems, IM_ARRAYSIZE(blendModeItems));

  ImGui::SeparatorText("Transform");
  ImGui::DragFloat3("Sphere Pos", &transform_.translate.x, 0.1f);
  ImGui::DragFloat3("Human Pos", &transformHuman_.translate.x, 0.1f);
  auto* editorCamera = dynamic_cast<AbsoluteEngine::EditorCamera*>(editorCamera_.get());
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

  // --- ポストプロセスタブ ---
  ImGui::Begin("PostProcess##Panel");
  ImGui::SeparatorText("PostProcess Settings");
  
  int ppMode = static_cast<int>(postProcessMode_);
  const char* postProcessItems[] = {
      "Normal", "Grayscale", "Sepia", "Vignette", "BoxFilter", 
      "GaussianFilter", "LuminanceOutline", "DepthOutline", "RadialBlur", "Dissolve", "Random", "HSV"
  };
  
  if (ImGui::Combo("Effect Mode", &ppMode, postProcessItems, IM_ARRAYSIZE(postProcessItems))) {
      postProcessMode_ = static_cast<Renderer::PostProcessMode>(ppMode);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (postProcessMode_ == Renderer::PostProcessMode::Vignette) {
      ImGui::SliderFloat("Vignette Scale", &vignetteScale_, 1.0f, 32.0f);
      ImGui::SliderFloat("Vignette Pow", &vignettePow_, 0.1f, 5.0f);
  } else if (postProcessMode_ == Renderer::PostProcessMode::BoxFilter) {
      ImGui::SliderInt("BoxFilter K", &boxFilterK_, 1, 10);
  } else if (postProcessMode_ == Renderer::PostProcessMode::GaussianFilter) {
      ImGui::SliderInt("GaussianFilter K", &gaussianFilterK_, 1, 10);
      ImGui::SliderFloat("GaussianFilter Sigma", &gaussianFilterSigma_, 0.1f, 10.0f);
  } else if (postProcessMode_ == Renderer::PostProcessMode::RadialBlur) {
      ImGui::SliderFloat2("Center", &radialBlurCenter_.x, 0.0f, 1.0f);
      ImGui::SliderFloat("Blur Width", &radialBlurWidth_, 0.0f, 0.1f);
  } else if (postProcessMode_ == Renderer::PostProcessMode::Dissolve) {
      ImGui::SliderFloat("Threshold", &dissolveThreshold_, 0.0f, 1.0f);
      ImGui::SliderFloat("Edge Range", &dissolveEdgeRange_, 0.0f, 0.1f);
      ImGui::ColorEdit3("Edge Color", &dissolveEdgeColor_.x);
      ImGui::ColorEdit3("Mask Color", &dissolveMaskColor_.x);
  } else if (postProcessMode_ == Renderer::PostProcessMode::HSV) {
      ImGui::SliderFloat("Hue", &hsvHue_, -1.0f, 1.0f);
      ImGui::SliderFloat("Saturation", &hsvSaturation_, -1.0f, 1.0f);
      ImGui::SliderFloat("Value", &hsvValue_, -1.0f, 1.0f);
  }

  // レンダラーにポストエフェクトのパラメータを渡す
  Renderer::GetInstance()->SetVignetteParam(vignetteScale_, vignettePow_);
  Renderer::GetInstance()->SetBoxFilterParam(boxFilterK_);
  Renderer::GetInstance()->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {1.0f, 0.0f});
  Renderer::GetInstance()->SetRadialBlurParam(radialBlurCenter_, radialBlurWidth_);
  Renderer::GetInstance()->SetDissolveParam(dissolveThreshold_, dissolveEdgeRange_, dissolveEdgeColor_, dissolveMaskColor_);
  Renderer::GetInstance()->SetRandomParam(time_);
  Renderer::GetInstance()->SetHSVParam(hsvHue_, hsvSaturation_, hsvValue_);
  
  ImGui::End();

  // --- ツールバー・エディタUIの描画（BaseScene側で行う） ---
  DrawEditorUI();

#endif

  ringTransform_.rotate.z += 1.5f * deltaTime;
  cylinderTransform_.rotate.y += 1.0f * deltaTime;

  // ゲームロジックは PlayMode の時のみ更新する
  if (playMode_ == PlayMode::Play) {


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
}

void DevScene::Draw() {
  auto* renderer = Renderer::GetInstance();
  auto* dx = renderer->GetDX();
  auto* cmdList = dx->GetCommandList();

  // --- カメラ・ライト設定（オフスクリーンパス前に確定させる） ---
  if (editorCamera_) {
    renderer->SetCamera(*editorCamera_);
  }
  renderer->SetEnvironmentMap(skybox_.GetTexture());
  renderer->SetVignetteParam(vignetteScale_, vignettePow_);
  // --- ライトの適用 ---
  ApplyEditorLightsToRenderer(renderer);

  // --- オフスクリーン描画パス（RenderTexture → ImGui::Image で表示） ---
  if (renderTexture_ && depthTexture_) {
    // RenderTexture に切り替え（SRV → RT バリア + OMSetRenderTargets）
    dx->SetRenderTargetWithDepth(renderTexture_.get(), depthTexture_.get());

    // RenderTexture をクリア（暗いグレー）
    float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    cmdList->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);

    // 深度バッファをクリア
    cmdList->ClearDepthStencilView(depthTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
          editorCamera_->GetViewMatrix(), editorCamera_->GetProjectionMatrix());
      ring_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f},
          MakeScaleMatrix({ringUVScale_.x, ringUVScale_.y, 1.0f}));
      if (texRing_) renderer->DrawRing(&ring_, texRing_->GetSrvGpu());

      cylinder_.SetTransform(
          MakeAffineMatrix(cylinderTransform_.scale, cylinderTransform_.rotate, cylinderTransform_.translate),
          editorCamera_->GetViewMatrix(), editorCamera_->GetProjectionMatrix());
      cylinder_.SetMaterial({1.0f, 1.0f, 1.0f, 1.0f},
          MakeScaleMatrix({cylinderUVScale_.x, cylinderUVScale_.y, 1.0f}));
      if (texCylinder_) renderer->DrawCylinder(&cylinder_, texCylinder_->GetSrvGpu());
    }

    for (auto& ef : hitEffects_) renderer->DrawEffectModel(&ef.instance);

    // GPUパーティクル描画
    renderer->DrawGPUParticles(static_cast<BlendMode>(particleBlendMode_ + 1));

    // RenderTexture を SRV 状態に戻す
    dx->FinishRenderingWithDepth(renderTexture_.get(), depthTexture_.get());
  }

  // --- ポストプロセスの適用 ---
  if (renderTexture_ && postProcessTexture_) {
    if (postProcessMode_ == Renderer::PostProcessMode::GaussianFilter && gaussianTempTexture_) {
      // パス1: 横方向
      dx->SetRenderTarget(gaussianTempTexture_.get());
      renderer->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {1.0f, 0.0f});
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(gaussianTempTexture_.get());

      // パス2: 縦方向
      dx->SetRenderTarget(postProcessTexture_.get());
      renderer->SetGaussianFilterParam(gaussianFilterK_, gaussianFilterSigma_, {0.0f, 1.0f});
      renderer->DrawFullscreen(gaussianTempTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(postProcessTexture_.get());
    } else if (postProcessMode_ == Renderer::PostProcessMode::DepthBasedOutline) {
      dx->SetRenderTarget(postProcessTexture_.get());
      if (editorCamera_) {
        renderer->SetDepthBasedOutlineParam(Inverse(editorCamera_->GetProjectionMatrix()));
      }
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_, depthTexture_->GetSrvGpuHandle());
      dx->FinishRendering(postProcessTexture_.get());
    } else if (postProcessMode_ == Renderer::PostProcessMode::Dissolve && texNoise0_) {
      dx->SetRenderTarget(postProcessTexture_.get());
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_, texNoise0_->GetSrvGpu());
      dx->FinishRendering(postProcessTexture_.get());
    } else {
      dx->SetRenderTarget(postProcessTexture_.get());
      renderer->DrawFullscreen(renderTexture_->GetSrvGpuHandle(), postProcessMode_);
      dx->FinishRendering(postProcessTexture_.get());
    }
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

  texNoise0_ = tm->Load("resources/noise/noise0.png");
  Renderer::GetInstance()->SetDissolveMaskTexture(texNoise0_);

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

  // --- ライトの初期設定 ---
  // 最初からシーンに配置しておくライトを rootObjects_ に追加する
  auto initialDirLight = std::make_shared<AbsoluteEngine::GameObject>("Directional Light");
  initialDirLight->GetTransform().rotate = { 0.5f, 0.5f, 0.0f }; // 適当な方向
  initialDirLight->GetLight().type = AbsoluteEngine::LightComponent::Type::Directional;
  initialDirLight->GetLight().color = { 1.0f, 1.0f, 1.0f };
  initialDirLight->GetLight().intensity = 1.0f;
  rootObjects_.push_back(initialDirLight);

  auto initialPointLight = std::make_shared<AbsoluteEngine::GameObject>("Point Light");
  initialPointLight->GetTransform().translate = { 0.0f, 2.0f, -2.0f };
  initialPointLight->GetLight().type = AbsoluteEngine::LightComponent::Type::Point;
  initialPointLight->GetLight().color = { 1.0f, 1.0f, 1.0f };
  initialPointLight->GetLight().intensity = 1.0f;
  initialPointLight->GetLight().radius = 10.0f;
  rootObjects_.push_back(initialPointLight);

  // オフスクリーンテスト初期化
  renderTexture_ = std::make_unique<RenderTexture>();
  renderTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {0.1f, 0.25f, 0.5f, 1.0f});

  depthTexture_ = std::make_unique<DepthTexture>();
  depthTexture_->Initialize(dx, 1280, 720);

  postProcessTexture_ = std::make_unique<RenderTexture>();
  postProcessTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {1,0,0,1});

  gaussianTempTexture_ = std::make_unique<RenderTexture>();
  gaussianTempTexture_->Initialize(dx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, {1,0,0,1});
}

void DevScene::InitCamera_() {
  transformTerrain_.translate = {0, -5.0f, 0};
  cameraTransform_.translate = {0, 0, -15};
  transformSimpleSkin_.translate = {3, 0, 0};
  transformHuman_.translate = {6, 0, 0};
  transformAnimCube_.translate = {-3, 0, 0};
  if (editorCamera_) editorCamera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);
}

void DevScene::SpawnHitEffect(const Vector3 &pos) {
  for (int i = 0; i < 8; ++i) {
    ParticleManager::GetInstance()->Emit(
        particleGroupName_, pos, Vector3{0, 0, 0}, Vector3{0.05f, 1.0f, 1.0f},
        Vector3{0, 0, (float)i}, 0.5f, Vector4{1, 1, 1, 1});
  }
}
