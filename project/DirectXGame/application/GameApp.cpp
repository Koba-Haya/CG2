#define NOMINMAX
#include "GameApp.h"
#include "DebugCamera.h"
#include "DirectXResourceUtils.h"
#include "ModelUtils.h"

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
  // 入力は Run() のループ先頭で更新済み

  // DebugCamera 更新
  if (debugCamera_) {
    debugCamera_->Update(input_);
  }

  // ===== ImGui フレーム開始 =====
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // ===== Settings ウィンドウ =====
  static bool settingsOpen = true;
  if (settingsOpen) {
    ImGui::Begin("Settings", &settingsOpen);

    // Light
    ImGui::Text("Light");

    if (directionalLightData_) {
      // 方向
      float lightDir[3] = {directionalLightData_->direction.x,
                           directionalLightData_->direction.y,
                           directionalLightData_->direction.z};
      if (ImGui::SliderFloat3("LightDirection", lightDir, -1.0f, 1.0f)) {
        directionalLightData_->direction = {lightDir[0], lightDir[1],
                                            lightDir[2]};
      }

      // 色
      ImGui::ColorEdit3("LightColor", reinterpret_cast<float *>(
                                          &directionalLightData_->color));

      // 強さ
      ImGui::SliderFloat("Intensity", &directionalLightData_->intensity, 0.0f,
                         5.0f);
    }

    // Sphere
    ImGui::Text("Sphere");
    ImGui::DragFloat3("SphereTranslate",
                      reinterpret_cast<float *>(&transform_.translate), 0.01f);
    ImGui::DragFloat3("SphereRotate",
                      reinterpret_cast<float *>(&transform_.rotate), 0.01f);
    ImGui::DragFloat3("SphereScale",
                      reinterpret_cast<float *>(&transform_.scale), 0.01f, 0.0f,
                      5.0f);

    // Plane
    ImGui::Text("Plane");
    ImGui::DragFloat3("PlaneTranslate",
                      reinterpret_cast<float *>(&transform2_.translate), 0.01f);
    ImGui::DragFloat3("PlaneRotate",
                      reinterpret_cast<float *>(&transform2_.rotate), 0.01f);
    ImGui::DragFloat3("PlaneScale",
                      reinterpret_cast<float *>(&transform2_.scale), 0.01f,
                      0.0f, 5.0f);

    // Sprite
    ImGui::Text("Sprite");
    ImGui::DragFloat3("SpriteTranslate",
                      reinterpret_cast<float *>(&transformSprite_.translate),
                      1.0f);
    ImGui::DragFloat3("SpriteRotate",
                      reinterpret_cast<float *>(&transformSprite_.rotate),
                      0.01f);
    ImGui::DragFloat3("SpriteScale",
                      reinterpret_cast<float *>(&transformSprite_.scale),
                      0.01f);

    // UV
    ImGui::Text("UV");
    ImGui::DragFloat2("UVTranslate", &uvTransformSprite_.translate.x, 0.01f,
                      -10.0f, 10.0f);
    ImGui::DragFloat2("UVScale", &uvTransformSprite_.scale.x, 0.01f, -10.0f,
                      10.0f);
    ImGui::SliderAngle("UVRotate", &uvTransformSprite_.rotate.z);

    // Lighting mode
    ImGui::Text("Lighting Mode");
    ImGui::RadioButton("None", &lightingMode_, 0);
    ImGui::RadioButton("Lambert", &lightingMode_, 1);
    ImGui::RadioButton("Half-Lambert", &lightingMode_, 2);

    // Sprite Blend mode
    const char *blendModeItems[] = {
        "Alpha (通常)",    "Add (加算)",          "Subtract (減算)",
        "Multiply (乗算)", "Screen (スクリーン)",
    };
    ImGui::Combo("Sprite Blend", &spriteBlendMode_, blendModeItems,
                 IM_ARRAYSIZE(blendModeItems));

    // Sound
    ImGui::Text("Sound");
    ImGui::SliderFloat("Volume", &selectVol_, 0.0f, 1.0f);
    if (ImGui::Button("Play")) {
      audio_.Play("select", false, selectVol_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      audio_.Stop("select");
    }

    ImGui::Separator();

    // ===== Particles =====
    ImGui::Text("Particles");

    ImGui::SliderInt("Count", reinterpret_cast<int *>(&particleCountUI_), 0,
                     static_cast<int>(kParticleCount_));

    ImGui::DragFloat3("Spawn Center",
                      reinterpret_cast<float *>(&particleSpawnCenter_), 0.1f);
    ImGui::DragFloat3("Spawn Extent",
                      reinterpret_cast<float *>(&particleSpawnExtent_), 0.1f,
                      0.0f, 100.0f);

    ImGui::DragFloat3("Base Dir", reinterpret_cast<float *>(&particleBaseDir_),
                      0.01f, -1.0f, 1.0f);
    ImGui::SliderFloat("Dir Random", &particleDirRandomness_, 0.0f, 1.0f);

    ImGui::SliderFloat("Speed Min", &particleSpeedMin_, 0.0f, 10.0f);
    ImGui::SliderFloat("Speed Max", &particleSpeedMax_, 0.0f, 10.0f);
    if (particleSpeedMin_ > particleSpeedMax_)
      particleSpeedMin_ = particleSpeedMax_;

    ImGui::SliderFloat("Life Min", &particleLifeMin_, 0.1f, 10.0f);
    ImGui::SliderFloat("Life Max", &particleLifeMax_, 0.1f, 10.0f);
    if (particleLifeMin_ > particleLifeMax_)
      particleLifeMin_ = particleLifeMax_;

    ImGui::End();
  }

  // ===== パーティクル行列更新 =====
  if (particleMatrices_) {
    // deltaTime（ちゃんと測るなら前フレーム時刻を記憶する）
    float deltaTime = 1.0f / 60.0f;

    // カメラ行列（後で DebugCamera に差し替え）
    Matrix4x4 viewMatrix = Inverse(MakeAffineMatrix(
        {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}));
    Matrix4x4 projMatrix = MakePerspectiveFovMatrix(
        0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
        100.0f);

    uint32_t activeCount = std::min(particleCountUI_, kParticleCount_);

    for (uint32_t i = 0; i < activeCount; ++i) {
      Particle &p = particles_[i];

      // 時間経過
      p.age += deltaTime;
      if (p.age >= p.lifetime) {
        // 寿命を超えたら再スポーン
        RespawnParticle_(p);
      }

      // 位置更新
      p.transform.translate.x += p.velocity.x * deltaTime;
      p.transform.translate.y += p.velocity.y * deltaTime;
      p.transform.translate.z += p.velocity.z * deltaTime;

      // フェードアウト (age/lifetime で 1→0)
      float t = p.age / p.lifetime;
      float alpha = std::clamp(1.0f - t, 0.0f, 1.0f);

      // パーティクル用マテリアルの alpha に反映
      if (particleMaterialData_) {
        particleMaterialData_->color.w = alpha;
      }

      // 行列計算
      Matrix4x4 world = MakeAffineMatrix(p.transform.scale, p.transform.rotate,
                                         p.transform.translate);
      Matrix4x4 wvp = Multiply(world, Multiply(viewMatrix, projMatrix));
      particleMatrices_[i].World = world;
      particleMatrices_[i].WVP = wvp;
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
  Matrix4x4 viewMatrix = Inverse(MakeAffineMatrix(
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}));
  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
      100.0f);

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
    // model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
    // planeModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
  }

  // ==============================
  // パーティクル描画（instancing）
  // ==============================
  {
    cmdList->SetGraphicsRootSignature(particlePipeline_.GetRootSignature());
    cmdList->SetPipelineState(particlePipeline_.GetPipelineState());

    D3D12_VERTEX_BUFFER_VIEW vbv = model_.GetVBV();
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    ID3D12DescriptorHeap *heaps[] = {dx_.GetSRVHeap()};
    cmdList->SetDescriptorHeaps(1, heaps);

    // RootParameter:
    // 0: PS b0 (Material)
    // 1: PS t0 (Texture)
    // 2: VS t1 (Instancing matrices)

    // パーティクル用 Material（PS:b0）
    cmdList->SetGraphicsRootConstantBufferView(
        0, particleMaterialCB_->GetGPUVirtualAddress());

    // Texture（PS:t0）: 今は model_ と同じテクスチャを使う
    cmdList->SetGraphicsRootDescriptorTable(1, model_.GetTextureHandle());

    // Instancing（VS:t1）
    cmdList->SetGraphicsRootDescriptorTable(2, particleMatricesSrvGPU_);

    // ビューポート / シザー
    cmdList->RSSetViewports(1, &dx_.GetViewport());
    cmdList->RSSetScissorRects(1, &dx_.GetScissorRect());

    // DrawInstanced
    UINT vertexCount = vbv.SizeInBytes / vbv.StrideInBytes;
    UINT instanceCount = std::min(particleCountUI_, kParticleCount_);
    cmdList->DrawInstanced(vertexCount, instanceCount, 0, 0);
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

  // 3D オブジェクト用
  {
    auto desc = UnifiedPipeline::MakeObject3DDesc();
    bool ok = objPipeline_.Initialize(device, dxcUtils, dxcCompiler,
                                      includeHandler, desc);
    assert(ok);
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
  assert(particlePipeline_.Initialize(device, dxcUtils, dxcCompiler,
                                      includeHandler, partDesc));
}

void GameApp::InitResources_() {
  ComPtr<ID3D12Device> device;
  dx_.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

  // ===== モデル =====
  {
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
  {
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
        device, sizeof(TransformationMatrix) * kParticleInstanceCount);

    particleInstanceBuffer_->Map(0, nullptr,
                                 reinterpret_cast<void **>(&particleMatrices_));
    for (uint32_t i = 0; i < kParticleInstanceCount; ++i) {
      particleMatrices_[i].WVP = MakeIdentity4x4();
      particleMatrices_[i].World = MakeIdentity4x4();
    }

    // SRV 作成（VS:t1 用 StructuredBuffer）
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = kParticleInstanceCount;
    desc.Buffer.StructureByteStride = sizeof(TransformationMatrix);
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    UINT index = srvAlloc_.Allocate();
    device->CreateShaderResourceView(particleInstanceBuffer_.Get(), &desc,
                                     srvAlloc_.Cpu(index));
    particleMatricesSrvGPU_ = srvAlloc_.Gpu(index);
  }

  // パーティクル Transform 初期化
  for (uint32_t i = 0; i < kParticleCount_; ++i) {
    RespawnParticle_(particles_[i]);
  }

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

// GameApp のプライベートメンバ関数として追加してOK
void GameApp::RespawnParticle_(Particle &p) {
  // 位置: 生成中心 ± Extent * ランダム[-1,1]
  auto rndSigned = []() { return Rand01() * 2.0f - 1.0f; };

  Vector3 offset{
      rndSigned() * particleSpawnExtent_.x,
      rndSigned() * particleSpawnExtent_.y,
      rndSigned() * particleSpawnExtent_.z,
  };
  p.transform.translate = {
      particleSpawnCenter_.x + offset.x,
      particleSpawnCenter_.y + offset.y,
      particleSpawnCenter_.z + offset.z,
  };

  // スケール（ここはとりあえず固定。ImGui
  // から調整したければ後でパラメータ追加）
  p.transform.scale = {0.5f, 0.5f, 0.5f};
  p.transform.rotate = {0.0f, 0.0f, 0.0f};

  // 方向: BaseDir ± ランダムなノイズ
  Vector3 dir = particleBaseDir_;
  dir.x += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  dir.y += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  dir.z += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
  if (Length(dir) > 0.0001f) {
    dir = Normalize(dir);
  } else {
    dir = {0.0f, 1.0f, 0.0f}; // デフォルト上向き
  }

  // 速度
  float speed = RandRange(particleSpeedMin_, particleSpeedMax_);
  p.velocity = {dir.x * speed, dir.y * speed, dir.z * speed};

  // 寿命と年齢
  p.lifetime = RandRange(particleLifeMin_, particleLifeMax_);
  p.age = 0.0f;
}