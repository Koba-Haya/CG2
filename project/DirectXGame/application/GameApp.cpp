#include "GameApp.h"
#include "ModelUtils.h"
#include "DirectXResourceUtils.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <objbase.h> // CoInitializeEx, CoUninitialize

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
  // TODO:
  // - DebugCamera 更新
  // - Transform / パーティクル更新
  // - ImGui の UI で値を変更（本当は GuiManager に寄せたい）
}

void GameApp::Draw() {
  dx_.BeginFrame();

  // TODO:
  // - RTV/DSV クリア
  // - ビューポート / シザー設定
  // - モデル / スプライト / パーティクル描画
  // - ImGui 描画

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
    ci.modelData = LoadObjFile("resources/fence", "fence.obj");
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
    particles_[i].transform.scale = {0.5f, 0.5f, 0.5f};
    particles_[i].transform.rotate = {0.0f, 0.0f, 0.0f};
    particles_[i].transform.translate = {0.0f + i * 0.1f, 0.0f + i * 0.1f,
                                         0.0f + i * 0.1f};
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