#define NOMINMAX
#include "GameApp.h"
#include "DebugCamera.h"
#include "DirectXResourceUtils.h"
#include "ModelUtils.h"
#include "TextureUtils.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <objbase.h> // CoInitializeEx, CoUninitialize
#include <random>

// ファイル先頭の方（名前空間外）に追加
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
  if (h < 0.0f)
    h += 1.0f;

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

GameApp::GameApp()
    : winApp_(), dx_(), input_(), audio_(), transform_{}, cameraTransform_{},
      transformSprite_{}, uvTransformSprite_{}, transform2_{}, particles_{} {}

GameApp::~GameApp() { Finalize(); }

bool GameApp::Initialize() {
  // COM 初期化
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  assert(SUCCEEDED(hr));

  // ログディレクトリ & ログファイル
  InitLogging_();

  // WinApp 初期化
  winApp_.Initialize();

  // DirectX 初期化（InitParams に合わせる）
  DirectXCommon::InitParams params{
      winApp_.GetHInstance(),
      winApp_.GetHwnd(),
      WinApp::kClientWidth,
      WinApp::kClientHeight,
  };
  dx_.Initialize(params);

  // Input 初期化
  bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
  assert(inputOk);

  // Audio 初期化
  bool audioOk = audio_.Initialize();
  assert(audioOk);

  // SRV 割り当てヘルパ初期化（DirectXCommon の SRV ヒープを利用）
  srvAlloc_.Init(dx_.GetDevice(), dx_.GetSRVHeap());

  // ShaderCompiler 初期化（DirectXCommon の DXC を流用）
  shaderCompiler_.Initialize(dx_.GetDXCUtils(), dx_.GetDXCCompiler(),
                             dx_.GetDXCIncludeHandler());

  // パイプライン初期化（Step3 のメイン）
  InitPipelines_();

  // リソース（モデル・スプライト・パーティクル等）初期化
  InitResources_();

  // カメラ関連
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

  // ウィンドウの×ボタンが押されるまでループ
  while (winApp_.ProcessMessage()) {
    // 入力更新
    input_.Update();

    // ゲーム更新
    Update();

    // 描画
    Draw();
  }

  Finalize();
  return 0;
}

void GameApp::Finalize() {
  static bool finalized = false;
  if (finalized)
    return;
  finalized = true;

  // TODO: DebugCamera delete, Resource 解放, ImGui 終了などを整理してここへ
  audio_.Shutdown();
  input_.Finalize();
  // dx_ はデストラクタで ImGui
  // などを後始末しているので、そのまま破棄に任せてもよい

  winApp_.Finalize();
  CoUninitialize();
}

void GameApp::Update() {
  // DebugCamera 更新
  if (debugCamera_) {
    debugCamera_->Update(input_);
    view3D_ = debugCamera_->GetViewMatrix();
  } else {
    // フォールバック：今まで通りの固定カメラ
    view3D_ = Inverse(MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
                                       {0.0f, 0.0f, -10.0f}));
  }

  // ← ここを if/else の外に出して、毎フレーム共通で計算
  proj3D_ = MakePerspectiveFovMatrix(
      0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
      100.0f);

  // ===== ImGui フレーム開始 =====
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // ===== Settings ウィンドウ =====
  static bool settingsOpen = true;
  if (settingsOpen) {
    ImGui::Begin("Settings", &settingsOpen);

    // ===== Particles =====
    ImGui::Text("Particles");

    // 最大発生数
    ImGui::SliderInt("最大発生数", reinterpret_cast<int *>(&particleCountUI_),
                     0, static_cast<int>(kParticleCount_));

    // 発生頻度（1秒あたりの発生数）
    ImGui::SliderFloat("発生頻度（1秒あたりの発生数）", &particleEmitRate_,
                       0.0f, 100.0f);
    if (particleEmitRate_ < 0.0f)
      particleEmitRate_ = 0.0f;

    // エミッタ位置・範囲
    ImGui::DragFloat3("エミッタ位置",
                      reinterpret_cast<float *>(&particleSpawnCenter_), 0.01f);
    ImGui::DragFloat3("エミッタ範囲",
                      reinterpret_cast<float *>(&particleSpawnExtent_), 0.01f,
                      0.0f, 10.0f);

    ImGui::DragFloat3("初速度", reinterpret_cast<float *>(&particleBaseDir_),
                      0.01f, -1.0f, 1.0f);
    ImGui::SliderFloat("初速度ランダム範囲", &particleDirRandomness_, 0.0f,
                       1.0f);

    // ----- 速度範囲 -----
    ImGui::SliderFloat("最低速度", &particleSpeedMin_, 0.0f, 10.0f);
    ImGui::SliderFloat("最大速度", &particleSpeedMax_, 0.0f, 10.0f);
    if (particleSpeedMin_ > particleSpeedMax_)
      particleSpeedMin_ = particleSpeedMax_;

    // ----- 寿命範囲 -----
    ImGui::SliderFloat("最低寿命", &particleLifeMin_, 0.1f, 10.0f);
    ImGui::SliderFloat("最大寿命", &particleLifeMax_, 0.1f, 10.0f);
    if (particleLifeMin_ > particleLifeMax_)
      particleLifeMin_ = particleLifeMax_;

    // エミッタ範囲表示
    ImGui::Checkbox("Emitter範囲を表示", &showEmitterGizmo_);

    // エミッタ形状選択
    int shapeIndex = (emitterShape_ == EmitterShape::Box) ? 0 : 1;
    const char *shapeItems[] = {"Box(AABB)", "Sphere"};
    ImGui::Combo("Emitter形状", &shapeIndex, shapeItems,
                 IM_ARRAYSIZE(shapeItems));
    emitterShape_ =
        (shapeIndex == 0) ? EmitterShape::Box : EmitterShape::Sphere;

    // ===== Particle Color =====
    ImGui::Separator();

    // カラーモード選択
    int colorModeIndex = static_cast<int>(particleColorMode_);
    const char *colorModeItems[] = {
        "Random RGB",
        "Range RGB",
        "Range HSV",
        "Fixed Color",
    };
    ImGui::Combo("Mode", &colorModeIndex, colorModeItems,
                 IM_ARRAYSIZE(colorModeItems));
    particleColorMode_ = static_cast<ParticleColorMode>(colorModeIndex);

    // 共通: 基本色
    ImGui::ColorEdit4("Base Color",
                      reinterpret_cast<float *>(&particleBaseColor_));

    // モードごとのパラメータ
    if (particleColorMode_ == ParticleColorMode::RangeRGB) {
      ImGui::SliderFloat3("RGB Range",
                          reinterpret_cast<float *>(&particleColorRangeRGB_),
                          0.0f, 1.0f);
    }

    if (particleColorMode_ == ParticleColorMode::RangeHSV) {
      ImGui::SliderFloat3(
          "Base HSV", reinterpret_cast<float *>(&particleBaseHSV_), 0.0f, 1.0f);
      ImGui::SliderFloat3("HSV Range",
                          reinterpret_cast<float *>(&particleHSVRange_), 0.0f,
                          1.0f);
    }

    ImGui::Text("Blend");

    // ----- パーティクルブレンドモード -----
    const char *particleBlendItems[] = {"Alpha (通常)", "Add (加算)",
                                        "Subtract (減算)", "Multiply (乗算)",
                                        "Screen (スクリーン)"};
    ImGui::Combo("Particle Blend", &particleBlendMode_, particleBlendItems,
                 IM_ARRAYSIZE(particleBlendItems));

    // ----- Acceleration Field -----
    ImGui::Text("Acceleration Field");
    // 場のon/off
    ImGui::Checkbox("加速度場を有効化", &enableAccelerationField_);
    ImGui::DragFloat3(
        "加速度", reinterpret_cast<float *>(&accelerationField_.acceleration),
        0.1f, -100.0f, 100.0f);
    ImGui::DragFloat3(
        "範囲Min", reinterpret_cast<float *>(&accelerationField_.area.min),
                      0.1f, -10.0f, 10.0f);
    ImGui::DragFloat3(
        "範囲Max", reinterpret_cast<float *>(&accelerationField_.area.max),
                      0.1f, -10.0f, 10.0f);

    ImGui::End();
  }

  // ===== パーティクル更新 & 行列更新 =====
  if (particleMatrices_) {
    float deltaTime = 1.0f / 60.0f;

    // UI上の最大数（Emitterの上限）と GPU 側の上限の最小値
    uint32_t capacity = std::min(particleCountUI_, kParticleCount_);

    // まず既存パーティクルを更新しながら、寿命切れは削除
    uint32_t gpuIndex = 0;

    for (auto it = particles_.begin(); it != particles_.end();) {
      Particle &p = *it;

      // 経過時間
      p.age += deltaTime;

      // 寿命超え → list から消す（Respawnしない）
      if (p.age >= p.lifetime) {
        it = particles_.erase(it);
        continue;
      }
      
      if (enableAccelerationField_) {
        // 加速度フィールド内にいるなら速度更新
        if (IsCollision(accelerationField_.area, p.transform.translate)) {
          p.velocity.x += accelerationField_.acceleration.x * deltaTime;
          p.velocity.y += accelerationField_.acceleration.y * deltaTime;
          p.velocity.z += accelerationField_.acceleration.z * deltaTime;
        }
      }

      // 位置更新
      p.transform.translate.x += p.velocity.x * deltaTime;
      p.transform.translate.y += p.velocity.y * deltaTime;
      p.transform.translate.z += p.velocity.z * deltaTime;

      // フェードアウト (age/lifetime で 1→0)
      float t = p.age / p.lifetime;
      float alpha = std::clamp(1.0f - t, 0.0f, 1.0f);

      // GPU インスタンスバッファへ書き込むのは capacity まで
      if (gpuIndex < capacity) {
        Matrix4x4 world = MakeBillboardMatrix(p.transform.scale,
                                              p.transform.translate, view3D_);
        Matrix4x4 wvp = Multiply(world, Multiply(view3D_, proj3D_));

        particleMatrices_[gpuIndex].World = world;
        particleMatrices_[gpuIndex].WVP = wvp;
        particleMatrices_[gpuIndex].color = {p.color.x, p.color.y, p.color.z,
                                             alpha};

        ++gpuIndex;
      }

      ++it;
    }

    // 現在 GPU に詰めた個数を保存（Drawで使う）
    activeParticleCount_ = gpuIndex;

    // ===== Emitter: 新規発生 =====
    // 発生上限（list側も capacity まで）
    uint32_t currentCount = static_cast<uint32_t>(particles_.size());
    uint32_t maxCount = capacity;

    if (particleEmitRate_ > 0.0f && currentCount < maxCount) {
      // EmitRate [個/秒] → このフレームで何個分か
      particleEmitAccum_ += particleEmitRate_ * deltaTime;

      // 蓄積が 1 を超えるごとに 1 個ずつ出す
      while (particleEmitAccum_ >= 1.0f && currentCount < maxCount) {
        Particle p{};
        RespawnParticle_(p); // Emitter パラメータに従って初期化
        particles_.push_back(p);

        particleEmitAccum_ -= 1.0f;
        ++currentCount;
      }
    } else {
      // 発生させない時は蓄積もクリアしておく
      particleEmitAccum_ = 0.0f;
    }
  }

  // 最後に ImGui を確定
  ImGui::Render();
}

void GameApp::Draw() {
  dx_.BeginFrame();
  auto *cmdList = dx_.GetCommandList();

  // ===== クリア色 =====
  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};

  // DSV
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      dx_.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

  // 現在のバックバッファRTV
  UINT backBufferIndex = dx_.GetSwapChain()->GetCurrentBackBufferIndex();
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx_.GetRTVHandle(backBufferIndex);

  // RTV / DSV 設定
  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  // クリア
  cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                 nullptr);

  // ビューポート / シザー
  cmdList->RSSetViewports(1, &dx_.GetViewport());
  cmdList->RSSetScissorRects(1, &dx_.GetScissorRect());

  // ==============================
  // 3D モデル描画（model_, planeModel_）
  // ==============================

  // カメラ行列（とりあえず DebugCamera はまだ使わず固定）
  Matrix4x4 viewMatrix = view3D_;
  Matrix4x4 projectionMatrix = proj3D_;

  // ワールド行列を更新
  Matrix4x4 worldSphere = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);
  model_.SetWorldTransform(worldSphere);
  model_.SetLightingMode(lightingMode_);

  Matrix4x4 worldPlane = MakeAffineMatrix(transform2_.scale, transform2_.rotate,
                                          transform2_.translate);
  planeModel_.SetWorldTransform(worldPlane);

  // ===== 平行光（InitResources_ で作った CB を使う） =====
  {
    // 必要ならここで色・方向・強さを更新（今は固定値のままでもOK）
    // directionalLightData_->direction = {...}; など

    // Object 用 PSO/RootSignature セット
    cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
    cmdList->SetPipelineState(objPipeline_.GetPipelineState());

    // モデル描画
    model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
    planeModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());

    // ===== エミッタ範囲表示（ワイヤーフレーム固定） =====
    if (showEmitterGizmo_) {
      if (emitterShape_ == EmitterShape::Box) {
        // Extent は半サイズなので 2 倍してスケールに使う
        Vector3 scale = {
            particleSpawnExtent_.x * 2.0f,
            particleSpawnExtent_.y * 2.0f,
            particleSpawnExtent_.z * 2.0f,
        };
        Matrix4x4 world =
            MakeAffineMatrix(scale, {0, 0, 0}, particleSpawnCenter_);
        emitterBoxModel_.SetWorldTransform(world);
        emitterBoxModel_.Draw(viewMatrix, projectionMatrix,
                              directionalLightCB_.Get());
      } else {
        // Sphere: Extent を各軸ごとの半径としてそのまま使う（楕円体）
        Vector3 radius = particleSpawnExtent_;

        const float kMinRadius = 0.001f;
        if (radius.x < kMinRadius)
          radius.x = kMinRadius;
        if (radius.y < kMinRadius)
          radius.y = kMinRadius;
        if (radius.z < kMinRadius)
          radius.z = kMinRadius;

        Vector3 scale = radius; // 半径 = スケール
        Matrix4x4 world =
            MakeAffineMatrix(scale, {0, 0, 0}, particleSpawnCenter_);
        emitterSphereModel_.SetWorldTransform(world);
        emitterSphereModel_.Draw(viewMatrix, projectionMatrix,
                                 directionalLightCB_.Get());
      }
    }

    // ==============================
    // パーティクル描画（instancing）
    // ==============================
    {
      // パイプライン選択は今まで通り
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

      cmdList->SetGraphicsRootSignature(
          currentParticlePipeline->GetRootSignature());
      cmdList->SetPipelineState(currentParticlePipeline->GetPipelineState());

      cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      cmdList->IASetVertexBuffers(0, 1, &particleVBView_);
      cmdList->IASetIndexBuffer(&particleIBView_);

      ID3D12DescriptorHeap *heaps[] = {dx_.GetSRVHeap()};
      cmdList->SetDescriptorHeaps(1, heaps);

      cmdList->SetGraphicsRootConstantBufferView(
          0, particleMaterialCB_->GetGPUVirtualAddress());
      cmdList->SetGraphicsRootDescriptorTable(1, particleTextureHandle_);
      cmdList->SetGraphicsRootDescriptorTable(2, particleMatricesSrvGPU_);

      UINT indexCount = 6;
      UINT instanceCount =
          activeParticleCount_; // ★ list→GPU に詰めた個数だけ描画
      cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
    }

    // ==============================
    // 2D スプライト描画
    // ==============================

    // 射影 : 画面ピクセル座標系
    Matrix4x4 view2D = MakeIdentity4x4();
    Matrix4x4 proj2D =
        MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth),
                               float(WinApp::kClientHeight), 0.0f, 1.0f);

    // スプライトの Transform 反映
    sprite_.SetPosition(transformSprite_.translate);
    sprite_.SetScale(transformSprite_.scale);
    sprite_.SetRotation(transformSprite_.rotate);

    // UV: scale, rotate.z, translate を使って 2D 用のアフィン行列を作る
    Matrix4x4 uvMat = MakeAffineMatrix(
        {uvTransformSprite_.scale.x, uvTransformSprite_.scale.y, 1.0f},
        {0.0f, 0.0f, uvTransformSprite_.rotate.z},
        {uvTransformSprite_.translate.x, uvTransformSprite_.translate.y, 0.0f});

    sprite_.SetUVTransform(uvMat);

    // 使用するパイプラインを設定（ImGui で選択）
    UnifiedPipeline *currentSpritePipeline = &spritePipelineAlpha_;
    switch (spriteBlendMode_) {
    case 0:
      currentSpritePipeline = &spritePipelineAlpha_;
      break;
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
    }

    sprite_.SetPipeline(currentSpritePipeline);
    // SRV ヒープ設定（Model と共用）
    ID3D12DescriptorHeap *heaps[] = {dx_.GetSRVHeap()};
    cmdList->SetDescriptorHeaps(1, heaps);

    // Sprite 描画
    // sprite_.Draw(view2D, proj2D);

    // ===== ImGui 描画 =====
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

    dx_.EndFrame();
  }
}

void GameApp::InitLogging_() {
  // ログのディレクトリを用意
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

  // 3D オブジェクト用（ベース）
  PipelineDesc objBase = UnifiedPipeline::MakeObject3DDesc();
  assert(objPipeline_.Initialize(device, dxcUtils, dxcCompiler, includeHandler,
                                 objBase));

  // ===== Emitter Gizmo 用：ワイヤーフレームのみ =====
  {
    auto wire = objBase;
    wire.alphaBlend = false;              // ブレンドなしでOK
    wire.enableDepth = true;              // 深度あり
    wire.cullMode = D3D12_CULL_MODE_NONE; // 全面表示したいなら NONE
    wire.fillMode = D3D12_FILL_MODE_WIREFRAME;
    assert(emitterGizmoPipelineWire_.Initialize(device, dxcUtils, dxcCompiler,
                                                includeHandler, wire));
  }

  // Sprite 用（各種ブレンドモード）
  PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

  auto sprAlpha = sprBase;
  sprAlpha.blendMode = BlendMode::Alpha;
  assert(spritePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler,
                                         includeHandler, sprAlpha));

  auto sprAdd = sprBase;
  sprAdd.blendMode = BlendMode::Add;
  assert(spritePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
                                       includeHandler, sprAdd));

  auto sprSub = sprBase;
  sprSub.blendMode = BlendMode::Subtract;
  assert(spritePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
                                       includeHandler, sprSub));

  auto sprMul = sprBase;
  sprMul.blendMode = BlendMode::Multiply;
  assert(spritePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
                                       includeHandler, sprMul));

  auto sprScr = sprBase;
  sprScr.blendMode = BlendMode::Screen;
  assert(spritePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler,
                                          includeHandler, sprScr));

  // Particle 用
  auto partDesc = UnifiedPipeline::MakeParticleDesc();

  // ===== Particle 用（ブレンド違いで複数） =====
  PipelineDesc partBase = UnifiedPipeline::MakeParticleDesc();

  auto pAlpha = partBase;
  pAlpha.blendMode = BlendMode::Alpha;
  assert(particlePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler,
                                           includeHandler, pAlpha));

  auto pAdd = partBase;
  pAdd.blendMode = BlendMode::Add;
  assert(particlePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
                                         includeHandler, pAdd));

  auto pSub = partBase;
  pSub.blendMode = BlendMode::Subtract;
  assert(particlePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
                                         includeHandler, pSub));

  auto pMul = partBase;
  pMul.blendMode = BlendMode::Multiply;
  assert(particlePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
                                         includeHandler, pMul));

  auto pScr = partBase;
  pScr.blendMode = BlendMode::Screen;
  assert(particlePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler,
                                            includeHandler, pScr));
}

void GameApp::InitResources_() {
  ComPtr<ID3D12Device> device;
  dx_.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

  // ===== モデル =====
  { // 球体モデル
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &objPipeline_;
    ci.srvAlloc = &srvAlloc_;
    ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
    ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci.lightingMode = 1;
    bool okModel = model_.Initialize(ci);
    assert(okModel);
  }
  { // 平面モデル
    Model::CreateInfo ci2{};
    ci2.dx = &dx_;
    ci2.pipeline = &objPipeline_;
    ci2.srvAlloc = &srvAlloc_;
    ci2.modelData = LoadObjFile("resources/plane", "plane.obj");
    ci2.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    ci2.lightingMode = 1;
    bool okPlane = planeModel_.Initialize(ci2);
    assert(okPlane);
  }
  { // エミッタ用球モデル
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.srvAlloc = &srvAlloc_;
    ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
    ci.baseColor = {0.3f, 0.8f, 1.0f, 0.3f}; // 半透明っぽい色
    ci.lightingMode = 0;
    bool ok = emitterSphereModel_.Initialize(ci);
    assert(ok);
  }
  { // エミッタ用ボックスモデル
    Model::CreateInfo ci{};
    ci.dx = &dx_;
    ci.pipeline = &emitterGizmoPipelineWire_;
    ci.srvAlloc = &srvAlloc_;
    ci.modelData = LoadObjFile("resources/cube", "cube.obj");
    ci.baseColor = {1.0f, 0.8f, 0.2f, 0.3f}; // 半透明っぽい黄色
    ci.lightingMode = 0;
    bool ok = emitterBoxModel_.Initialize(ci);
    assert(ok);
  }
  // ===== Sprite =====
  {
    Sprite::CreateInfo sprInfo{};
    sprInfo.dx = &dx_;
    sprInfo.pipeline = &spritePipelineAlpha_;
    sprInfo.srvAlloc = &srvAlloc_;
    sprInfo.texturePath = "resources/uvChecker.png";
    sprInfo.size = {640.0f, 360.0f};
    sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};
    bool okSprite = sprite_.Initialize(sprInfo);
    assert(okSprite);
  }

  // ===== Particle Instancing Buffer =====
  {
    const uint32_t kParticleInstanceCount = kParticleCount_;

    particleInstanceBuffer_ = CreateBufferResource(
        device, sizeof(ParticleForGPU) * kParticleInstanceCount);

    particleInstanceBuffer_->Map(0, nullptr,
                                 reinterpret_cast<void **>(&particleMatrices_));
    for (uint32_t i = 0; i < kParticleInstanceCount; ++i) {
      particleMatrices_[i].WVP = MakeIdentity4x4();
      particleMatrices_[i].World = MakeIdentity4x4();
      particleMatrices_[i].color = {1, 1, 1, 1};
    }

    // SRV 作成（VS:t1 用 StructuredBuffer）
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = kParticleInstanceCount;
    desc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    UINT index = srvAlloc_.Allocate();
    device->CreateShaderResourceView(particleInstanceBuffer_.Get(), &desc,
                                     srvAlloc_.Cpu(index));
    particleMatricesSrvGPU_ = srvAlloc_.Gpu(index);
  }

  // パーティクル初期生成（初期 30 個）
  particles_.clear();
  uint32_t initialCount = std::min(initialParticleCount_, kParticleCount_);
  for (uint32_t i = 0; i < initialCount; ++i) {
    Particle p{};
    RespawnParticle_(p); // Emitter パラメータに従って初期化
    particles_.push_back(p);
  }
  activeParticleCount_ = initialCount; // とりあえず初期値

  // ===== Particle Material CB (PS:b0) =====
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
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&particleMaterialCB_));
    assert(SUCCEEDED(hr));

    particleMaterialCB_->Map(0, nullptr,
                             reinterpret_cast<void **>(&particleMaterialData_));

    // 初期値: 白＋アルファ1、ライティング無効、UV=単位行列
    particleMaterialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    particleMaterialData_->enableLighting = 0;
    particleMaterialData_->pad[0] = particleMaterialData_->pad[1] =
        particleMaterialData_->pad[2] = 0.0f;
    particleMaterialData_->uvTransform = MakeIdentity4x4();
  }

  // ===== パーティクル専用テクスチャ読み込み =====
  {
    // 1) 画像を読み込んで mip 生成
    DirectX::ScratchImage mipImages =
        LoadTexture("resources/particle/circle.png");
    const DirectX::TexMetadata &meta = mipImages.GetMetadata();

    // 2) GPU テクスチャリソースを作成
    particleTexture_ = CreateTextureResource(device, meta);
    UploadTextureData(particleTexture_, mipImages);

    // 3) SRV を作成して SRV ヒープに登録
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = meta.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);

    UINT texIndex = srvAlloc_.Allocate();
    device->CreateShaderResourceView(particleTexture_.Get(), &srvDesc,
                                     srvAlloc_.Cpu(texIndex));

    // 4) GPU ハンドルを保持（描画時に使う）
    particleTextureHandle_ = srvAlloc_.Gpu(texIndex);
  }

  // ===== DirectionalLight 用 CB (PS:b1) =====
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
    assert(SUCCEEDED(hr));

    directionalLightCB_->Map(0, nullptr,
                             reinterpret_cast<void **>(&directionalLightData_));

    // 初期値
    *directionalLightData_ = {{1, 1, 1, 1}, {0, -1, 0}, 1.0f};
  }

  // サウンド読み込み
  bool select = audio_.Load("select", L"resources/sound/select.mp3", 1.0f);
  assert(select);
  selectVol_ = 1.0f;

  // 2x2 の単位板ポリ（テクスチャは 0..1）
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

  // VB
  ComPtr<ID3D12Resource> particleVB, particleIB;
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
    HRESULT hr = device->CreateCommittedResource(
        &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&particleVB));
    assert(SUCCEEDED(hr));
    void *mapped = nullptr;
    particleVB->Map(0, nullptr, &mapped);
    memcpy(mapped, quad, vbSize);
    particleVB->Unmap(0, nullptr);
    particleVBView_.BufferLocation = particleVB->GetGPUVirtualAddress();
    particleVBView_.StrideInBytes = sizeof(Vtx);
    particleVBView_.SizeInBytes = static_cast<UINT>(vbSize);
    particleVB_ = particleVB; // メンバに保持
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
    HRESULT hr = device->CreateCommittedResource(
        &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&particleIB));
    assert(SUCCEEDED(hr));
    void *mapped = nullptr;
    particleIB->Map(0, nullptr, &mapped);
    memcpy(mapped, idx, ibSize);
    particleIB->Unmap(0, nullptr);
    particleIBView_.BufferLocation = particleIB->GetGPUVirtualAddress();
    particleIBView_.Format = DXGI_FORMAT_R16_UINT;
    particleIBView_.SizeInBytes = static_cast<UINT>(ibSize);
    particleIB_ = particleIB; // メンバに保持
  }
}

void GameApp::InitCamera_() {
  // Transform の初期値（main.cpp の初期化を踏襲）
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

  transformSprite_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  transform2_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}};

  // DebugCamera 生成（クラス定義に合わせて後で include & new）
  debugCamera_ = new DebugCamera();
}

Vector4 GameApp::GenerateParticleColor_() {
  Vector3 rgb{1.0f, 1.0f, 1.0f};

  switch (particleColorMode_) {
  case ParticleColorMode::RandomRGB: {
    rgb = {Rand01(), Rand01(), Rand01()};
    break;
  }
  case ParticleColorMode::RangeRGB: {
    auto randSigned = []() { return Rand01() * 2.0f - 1.0f; };
    rgb.x = particleBaseColor_.x + randSigned() * particleColorRangeRGB_.x;
    rgb.y = particleBaseColor_.y + randSigned() * particleColorRangeRGB_.y;
    rgb.z = particleBaseColor_.z + randSigned() * particleColorRangeRGB_.z;
    rgb.x = std::clamp(rgb.x, 0.0f, 1.0f);
    rgb.y = std::clamp(rgb.y, 0.0f, 1.0f);
    rgb.z = std::clamp(rgb.z, 0.0f, 1.0f);
    break;
  }
  case ParticleColorMode::RangeHSV: {
    auto randSigned = []() { return Rand01() * 2.0f - 1.0f; };
    Vector3 hsv = particleBaseHSV_;
    hsv.x += randSigned() * particleHSVRange_.x; // H
    hsv.y += randSigned() * particleHSVRange_.y; // S
    hsv.z += randSigned() * particleHSVRange_.z; // V

    // H は 0〜1 でループ
    hsv.x = std::fmod(hsv.x, 1.0f);
    if (hsv.x < 0.0f)
      hsv.x += 1.0f;
    hsv.y = std::clamp(hsv.y, 0.0f, 1.0f);
    hsv.z = std::clamp(hsv.z, 0.0f, 1.0f);

    rgb = HSVtoRGB(hsv);
    break;
  }
  case ParticleColorMode::Fixed: {
    rgb = {particleBaseColor_.x, particleBaseColor_.y, particleBaseColor_.z};
    break;
  }
  }

  return {rgb.x, rgb.y, rgb.z, particleBaseColor_.w}; // alphaはBaseのAを使う
}

// GameApp のプライベートメンバ関数として追加してOK
void GameApp::RespawnParticle_(Particle &p) {
  auto rndSigned = []() { return Rand01() * 2.0f - 1.0f; };

  Vector3 pos = particleSpawnCenter_;

  if (emitterShape_ == EmitterShape::Box) {
    // === AABB 的な箱内ランダム ===
    Vector3 offset{
        rndSigned() * particleSpawnExtent_.x,
        rndSigned() * particleSpawnExtent_.y,
        rndSigned() * particleSpawnExtent_.z,
    };
    pos.x += offset.x;
    pos.y += offset.y;
    pos.z += offset.z;
  } else { // EmitterShape::Sphere
    // === 球/楕円体内ランダム ===
    // Extent を各軸の半径として扱う
    Vector3 radius = particleSpawnExtent_;

    const float kMinRadius = 0.001f;
    if (radius.x < kMinRadius)
      radius.x = kMinRadius;
    if (radius.y < kMinRadius)
      radius.y = kMinRadius;
    if (radius.z < kMinRadius)
      radius.z = kMinRadius;

    // まず単位球内のランダム点を取る
    Vector3 local{};
    while (true) {
      local.x = RandRange(-1.0f, 1.0f);
      local.y = RandRange(-1.0f, 1.0f);
      local.z = RandRange(-1.0f, 1.0f);
      float len2 = local.x * local.x + local.y * local.y + local.z * local.z;
      if (len2 <= 1.0f) {
        break; // 単位球の中
      }
    }

    // 各軸ごとの半径でスケールして楕円体に射影
    pos.x += local.x * radius.x;
    pos.y += local.y * radius.y;
    pos.z += local.z * radius.z;
  }

  p.transform.translate = pos;

  // 以降（scale/rotate/dir/velocity/life/color）は今まで通り
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

// （ヘルパ追加）パーティクル用ビルボード行列生成
Matrix4x4 GameApp::MakeBillboardMatrix(const Vector3 &scale,
                                       const Vector3 &translate,
                                       const Matrix4x4 &viewMatrix) {
  Matrix4x4 camWorld = Inverse(viewMatrix);
  Vector3 right{camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2]};
  Vector3 up{camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2]};
  Vector3 forward{camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2]};
  // カメラへ向けるため forward を反転
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
  if (point.x < aabb.min.x || point.x > aabb.max.x) {
    return false;
  }
  if (point.y < aabb.min.y || point.y > aabb.max.y) {
    return false;
  }
  if (point.z < aabb.min.z || point.z > aabb.max.z) {
    return false;
  }
  return true;
}
