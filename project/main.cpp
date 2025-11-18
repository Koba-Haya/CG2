#include "DirectXCommon.h"
#include "Model.h"
#include "Sprite.h"
#include "TextureUtils.h"
#include "UnifiedPipeline.h"
#include "WinApp.h"
#include "audio/Audio.h"
#include "include.h"
#include <Windows.h>
#include <format>

struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

struct TransformationMatrix {
  Matrix4x4 WVP;
  Matrix4x4 World;
};

// チャンクヘッダ
struct ChunkHeader {
  char id[4];   // チャンクごとのID
  int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
  ChunkHeader chunk; // "RIFF"
  char type[4];      // "WAVE"
};

// FMTチャンク
struct FormatChunk {
  ChunkHeader chunk; // "fmt"
  WAVEFORMATEX fmt;  // 波形フォーマット
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

void Log(std::ostream &os, const std::string &message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
}

/* コンパイルシェーダー関数
-----------------------------------*/
IDxcBlob *CompileShader(
    // CompilerするShaderファイルへのパス
    const std::wstring &filePath,
    // Compilerに使用するProfile
    const wchar_t *profile,
    // 初期化で生成したものを三つ
    IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
    IDxcIncludeHandler *includeHandler);

/*Resource作成の関数化
-----------------------------*/
Microsoft::WRL::ComPtr<ID3D12Resource>
CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                     size_t sizeInBytes);

// SrvAllocator から使うための前方宣言（※定義はこの後ろの方にあります）
D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index);

// 文字列を格納する
std::string str0{"STRING!!!"};

// 整数を文字列にする
std::string str1{std::to_string(10)};

// 現在時刻を取得(UTC時刻)
std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

// 日本時間(PCの設定時間)に変換
std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};

// formatを使って年月日_時分秒の文字列に変換
std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);

// 時刻を使ってファイル名を決定
std::string logFilePath = std::string("logs/") + dateString + ".log";

// ファイルを作って書き込み準備
std::ofstream lobStream(logFilePath);

/* DescriptorHeapの作成関数　
------------------------------------------*/
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                     D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                     bool shaderVisible);

/* Textureデータを読む
-----------------------------*/
DirectX::ScratchImage LoadTexture(const std::string &filePath);

/* DirectX12のTextureResourceを作る
--------------------------------------*/
Microsoft::WRL::ComPtr<ID3D12Resource>
CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                      const DirectX::TexMetadata &metadata);

/* TextureResourceにデータを転送する
----------------------------------------*/
void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                       const DirectX::ScratchImage &mipImages);

/* DepthStencilTextureを作る
------------------------------------*/
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(
    const Microsoft::WRL::ComPtr<ID3D12Device> &device, INT32 width,
    INT32 height);

D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index);

ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename);

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename);

struct D3DResourceLeakChecker {
  ~D3DResourceLeakChecker() {
    // リソースリークチェック
    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
      debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
    }
  }
};

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  /* ReportLiveObjects
  ----------------------------*/
  D3DResourceLeakChecker leakCheck;

  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  assert(SUCCEEDED(hr));

  WinApp app;
  app.Initialize();

  Input input;
  bool inputOk = input.Initialize(app.GetHInstance(), app.GetHwnd());
  assert(inputOk);

  // 以降で必要なハンドルは WinApp 経由で取得
  HWND hwnd = app.GetHwnd();

  DirectXCommon dx;
  dx.Initialize(
      {app.GetHInstance(), hwnd, app.kClientWidth, app.kClientHeight});

  Microsoft::WRL::ComPtr<ID3D12Device> device = dx.GetDevice();
  /*Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx.GetCommandList();
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap =
      dx.GetSRVHeap();
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = dx.GetSwapChain();*/

  // ウィンドウ初期化
  MSG msg{};

/* デバッグレイヤー
------------------------*/
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    // デバッグレイヤーを有効化する
    debugController->EnableDebugLayer();
    // さらにGPU側でもチェックを行うようにする
    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  // ログのディレクトリを用意
  std::filesystem::create_directory("logs");

  /* エラー・警告、即ち停止
  ----------------------------*/
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // やばいエラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // エラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    // 警告時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    /*エラーと警告の抑制
    ---------------------------*/
    // 抑制するメッセージのID
    D3D12_MESSAGE_ID denyIds[] = {
        // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用によるエラーメッセージ
        // https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
    // 抑制するレベル
    D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    // 指定したメッセージの表示を抑制する
    infoQueue->PushStorageFilter(&filter);
    //  解放
    // infoQueue->Release();
  }
#endif

  /* Audio変数の宣言
  --------------------------*/

  AudioManager audio; // ← WinMain 冒頭で宣言
  bool audioOk = audio.Initialize();
  assert(audioOk);

  //================================
  //              PSO
  //================================

  // DXC は DirectXCommon から借りる
  auto *dxcUtils = dx.GetDXCUtils();
  auto *dxcCompiler = dx.GetDXCCompiler();
  auto *includeHandler = dx.GetDXCIncludeHandler();

  UnifiedPipeline objPipeline;
  auto objDesc = UnifiedPipeline::MakeObject3DDesc();
  bool ok = objPipeline.Initialize(dx.GetDevice(), dxcUtils, dxcCompiler,
                                   includeHandler, objDesc);
  assert(ok);

  UnifiedPipeline spritePipelineAlpha;
  UnifiedPipeline spritePipelineAdd;
  UnifiedPipeline spritePipelineSub;
  UnifiedPipeline spritePipelineMul;
  UnifiedPipeline spritePipelineScreen;

  // ベースとなる Sprite 設定
  PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

  // Alpha
  PipelineDesc sprAlpha = sprBase;
  sprAlpha.blendMode = BlendMode::Alpha;
  bool okSprAlpha = spritePipelineAlpha.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, sprAlpha);
  assert(okSprAlpha);

  // Add
  PipelineDesc sprAdd = sprBase;
  sprAdd.blendMode = BlendMode::Add;
  bool okSprAdd = spritePipelineAdd.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, sprAdd);
  assert(okSprAdd);

  // Subtract
  PipelineDesc sprSub = sprBase;
  sprSub.blendMode = BlendMode::Subtract;
  bool okSprSub = spritePipelineSub.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, sprSub);
  assert(okSprSub);

  // Multiply
  PipelineDesc sprMul = sprBase;
  sprMul.blendMode = BlendMode::Multiply;
  bool okSprMul = spritePipelineMul.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, sprMul);
  assert(okSprMul);

  // Screen
  PipelineDesc sprScr = sprBase;
  sprScr.blendMode = BlendMode::Screen;
  bool okSprScr = spritePipelineScreen.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, sprScr);
  assert(okSprScr);

  static int spriteBlendMode = 0; // 0=Alpha,1=Add,2=Sub,3=Mul,4=Screen

  //================================
  //              モデル
  //================================
  Model model;

  // SRV割り当てヘルパ（既存のヒープを流用）
  SrvAllocator srvAlloc;
  srvAlloc.Init(dx.GetDevice(),
                dx.GetSRVHeap()); // DirectXCommonのSRVヒープを使用

  // モデル作成（CreateInfoで1個に集約）
  Model::CreateInfo ci{};
  ci.dx = &dx;
  ci.pipeline = &objPipeline; // ← 生成済みの 3D用パイプライン
  ci.srvAlloc = &srvAlloc;
  ci.modelData = LoadObjFile("resources/fence", "fence.obj");
  ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  ci.lightingMode = 1; // 0:Unlit 1:Lambert 2:HalfLambert

  bool okModel = model.Initialize(ci);
  assert(okModel);

  Model planeModel;
  Model::CreateInfo ci2{};
  ci2.dx = &dx;
  ci2.pipeline = &objPipeline;
  ci2.srvAlloc = &srvAlloc;
  ci2.modelData = LoadObjFile("resources/plane", "plane.obj");
  ci2.baseColor = {1.0f, 1.0f, 1.0f, 1.0f}; // 色だけ変えるなど
  ci2.lightingMode = 1;
  planeModel.Initialize(ci2);

    // ===================== Particle Pipeline =====================
  UnifiedPipeline particlePipeline;
  auto partDesc = UnifiedPipeline::MakeParticleDesc();
  bool okParticle = particlePipeline.Initialize(
      dx.GetDevice(), dxcUtils, dxcCompiler, includeHandler, partDesc);
  assert(okParticle);

  // ===================== Instancing Buffer =====================
  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };
  const uint32_t kParticleInstanceCount = 10;

  // 行列用アップロードバッファ（StructuredBuffer用）
  Microsoft::WRL::ComPtr<ID3D12Resource> particleInstanceBuffer =
      CreateBufferResource(device, sizeof(TransformationMatrix) *
                                       kParticleInstanceCount);

  TransformationMatrix *particleMatrices = nullptr;
  particleInstanceBuffer->Map(0, nullptr,
                              reinterpret_cast<void **>(&particleMatrices));
  for (uint32_t i = 0; i < kParticleInstanceCount; ++i) {
    particleMatrices[i].WVP = MakeIdentity4x4();
    particleMatrices[i].World = MakeIdentity4x4();
  }

  // SRV 作成（t1 用 StructureBuffer）
  D3D12_SHADER_RESOURCE_VIEW_DESC particleInstancingSrvDesc{};
  particleInstancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
  particleInstancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  particleInstancingSrvDesc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  particleInstancingSrvDesc.Buffer.FirstElement = 0;
  particleInstancingSrvDesc.Buffer.NumElements = kParticleInstanceCount;
  particleInstancingSrvDesc.Buffer.StructureByteStride =
      sizeof(TransformationMatrix);
  particleInstancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

  UINT particleMatricesSrvIndex = srvAlloc.Allocate();
  device->CreateShaderResourceView(particleInstanceBuffer.Get(),
                                   &particleInstancingSrvDesc,
                                   srvAlloc.Cpu(particleMatricesSrvIndex));
  D3D12_GPU_DESCRIPTOR_HANDLE particleMatricesSrvGPU =
      srvAlloc.Gpu(particleMatricesSrvIndex);

  //================================
  //            スプライト
  //================================
  // Sprite 用オブジェクト
  Sprite sprite;
  Sprite::CreateInfo sprInfo{};
  sprInfo.dx = &dx;
  sprInfo.pipeline = &spritePipelineAlpha; // ✅ Spriteパイプラインを渡す！
  sprInfo.srvAlloc = &srvAlloc;
  sprInfo.texturePath = "resources/plane/uvChecker.png";
  sprInfo.size = {640.0f, 360.0f};
  sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};
  sprite.Initialize(sprInfo);

  ModelData modelData;
  // 左上
  modelData.vertices.push_back({.position = {1.0f, 1.0f, 0.0f},
                                .texcoord = {0.0f, 0.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  // 右上
  modelData.vertices.push_back({.position = {-1.0f, 1.0f, 0.0f},
                                .texcoord = {1.0f, 0.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  // 右下
  modelData.vertices.push_back({.position = {-1.0f, -1.0f, 0.0f},
                                .texcoord = {1.0f, 1.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  // 左下
  modelData.vertices.push_back({.position = {1.0f, -1.0f, 0.0f},
                                .texcoord = {0.0f, 1.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  // 右上
  modelData.vertices.push_back({.position = {-1.0f, 1.0f, 0.0f},
                                .texcoord = {1.0f, 0.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  // 右下
  modelData.vertices.push_back({.position = {-1.0f, -1.0f, 0.0f},
                                .texcoord = {1.0f, 1.0f},
                                .normal = {0.0f, 0.0f, 1.0f}});
  modelData.material.textureFilePath =
      "resources/uvChecker.png";

  const uint32_t kNumInstance = 10;
  // Instancing用のTransformationMatrixリソースを作る
  //Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource =
  //    CreateBufferResource(device, sizeof(TransformationMatrix) * kNumInstance);
  //// 書き込むためのアドレスを取得
  //TransformationMatrix *instancingData = nullptr;
  //instancingResource->Map(0, nullptr,
  //                        reinterpret_cast<void **>(&instancingData));
  //// 単位行列を書き込んでおく
  //for (uint32_t i = 0; i < kNumInstance; ++i) {
  //  instancingData[i].WVP = MakeIdentity4x4();
  //  instancingData[i].World = MakeIdentity4x4();
  //}

  //D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
  //instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
  //instancingSrvDesc.Shader4ComponentMapping =
  //    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  //instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  //instancingSrvDesc.Buffer.FirstElement = 0;
  //instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
  //instancingSrvDesc.Buffer.NumElements = kNumInstance;
  //instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);
  //D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU =
  //    GetCPUDescriptorHandle(srvAlloc.GetDescriptorHeap(),
  //                           srvAlloc.GetDescriptorSize(), srvAlloc.Alloc());
  //D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
  //    GetGPUDescriptorHandle(srvAlloc.GetDescriptorHeap(),
  //                           srvAlloc.GetDescriptorSize(), srvAlloc.Alloc());
  //device->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc,
  //                                 instancingSrvHandleCPU);

  // ===================== 3Dオブジェクト用 =====================
  auto rootSignature3D = objPipeline.GetRootSignature();
  auto pso3D = objPipeline.GetPipelineState();

  /*ViewportとScissor
  -------------------------*/
  // ビューポート
  D3D12_VIEWPORT viewport{};
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = app.kClientWidth;
  viewport.Height = app.kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  // シザー矩形
  D3D12_RECT scissorRect{};
  // 基本的にビューポートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = app.kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = app.kClientHeight;

  /* 平行光源をShaderで使う
  --------------------------------------*/
  // どこか初期化直後に（PS:b1用）
  auto directionalLightResource =
      CreateBufferResource(device.Get(), sizeof(DirectionalLight));
  DirectionalLight *directionalLightData = nullptr;
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));
  *directionalLightData = {{1, 1, 1, 1}, {0, -1, 0}, 1.0f};

  /*           初期化
  --------------------------------------------*/

  // Transform変数を作る
  Transform transform{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Transform cameraTransform{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};
  Transform transformSprite{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Transform uvTransformSprite{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Transform transform2 = {
      {1.0f, 1.0f, 1.0f}, // scale
      {0.0f, 0.0f, 0.0f}, // rotate
      {3.0f, 0.0f, 0.0f}  // translate（球の横に表示）
  };

  // ==== パーティクル用 Transform 配列 ====
  struct Particle {
    Transform transform;
  };

  static const uint32_t kParticleCount = 10;
  Particle particles[kParticleCount];

  for (uint32_t i = 0; i < kParticleCount; ++i) {
    particles[i].transform.scale = {0.5f, 0.5f, 0.5f};
    particles[i].transform.rotate = {0.0f, 0.0f, 0.0f};
    // X方向に等間隔で並べる（見えやすいように）
    particles[i].transform.translate = {0.0f + i * 0.1f, 0.0f + i * 0.1f,
                                        0.0f + i * 0.1f};
  }

  /*Transform transforms[kNumInstance];
  for (uint32_t i = 0; i < kNumInstance; ++i) {
    transforms[i].scale = {1.0f, 1.0f, 1.0f};
    transforms[i].rotate = {0.0f, 0.0f, 0.0f};
    transforms[i].translate = {i * 0.1f, i * 0.1f, i * 0.1f};
  }*/

  bool useMonsterBall = true;

  static int lightingMode = 1; // 初期値: Lambert

  // 音声読み込み
  bool select =
      audio.Load("select", L"resources/sound/select.mp3", 1.0f); // mp3等もOK
  assert(select);
  static float selectVol = 1.0f;

  DebugCamera *debugCamera = new DebugCamera();

  // ImGuiの初期化

  // ウィンドウの×ボタンが押されるまでループ
  while (app.ProcessMessage()) {

    // ゲームの処理
    dx.BeginFrame();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList =
        dx.GetCommandList();

    /* ID3D12DescriptorHeap *heaps[] = {srvDescriptorHeap.Get()};
     commandList->SetDescriptorHeaps(1, heaps);*/

    // ImGuiにフレームが始まることを伝える
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // パラメーターを変更 ImGuiの処理
    // ImGui::ShowDemoWindow(); //
    // 開発者用UIの処理。UIを出す場合はここをゲーム固有の処理に置き換える
    static bool settingsOpen = true;
    if (settingsOpen) {
      ImGui::Begin("Settings", &settingsOpen);

      ImGui::Text("Light");

      // ライトの方向
      static float lightDir[3] = {directionalLightData->direction.x,
                                  directionalLightData->direction.y,
                                  directionalLightData->direction.z};
      if (ImGui::SliderFloat3("LightDirection", lightDir, -1.0f, 1.0f)) {
        Vector3 dir = {lightDir[0], lightDir[1], lightDir[2]};
        directionalLightData->direction = Normalize(dir);
      }

      // ライトの色
      ImGui::ColorEdit3("LightColor", reinterpret_cast<float *>(
                                          &directionalLightData->color));

      // ライトの強さ
      ImGui::SliderFloat("Intensity", &directionalLightData->intensity, 0.0f,
                         5.0f);

      // マテリアル色（modelへ反映）
      static float col[3] = {1.0f, 1.0f, 1.0f};
      if (ImGui::ColorEdit3("SphereColor", col)) {
        model.SetColor({col[0], col[1], col[2], 1.0f});
      }

      // マテリアル色（modelへ反映）
      static float color[3] = {1.0f, 1.0f, 1.0f};
      if (ImGui::ColorEdit3("PlaneColor", color)) {
        planeModel.SetColor({color[0], color[1], color[2], 1.0f});
      }

      static float spriteColor[3] = {1.0f, 1.0f, 1.0f};
      if (ImGui::ColorEdit3("SpriteColor", spriteColor)) {
        sprite.SetColor({spriteColor[0], spriteColor[1], spriteColor[2], 1.0f});
      }

      // Lighting切り替え
      ImGui::Text("Lighting Mode");
      ImGui::RadioButton("None", &lightingMode, 0);
      ImGui::RadioButton("Lambert", &lightingMode, 1);
      ImGui::RadioButton("Half-Lambert", &lightingMode, 2);

      const char *blendModeItems[] = {
          "Alpha (通常)",    "Add (加算)",          "Subtract (減算)",
          "Multiply (乗算)", "Screen (スクリーン)",
      };

      ImGui::Combo("Sprite Blend", &spriteBlendMode, blendModeItems,
                   IM_ARRAYSIZE(blendModeItems));

      // カメラ
      ImGui::Text("Camera");
      ImGui::DragFloat3("CameraTranslate",
                        reinterpret_cast<float *>(&cameraTransform.translate),
                        0.01f);
      ImGui::DragFloat3("CameraRotate",
                        reinterpret_cast<float *>(&cameraTransform.rotate),
                        0.01f);

      // 球
      ImGui::Text("Sphere");

      ImGui::DragFloat3("SphereTranslate",
                        reinterpret_cast<float *>(&transform.translate), 0.01f);
      ImGui::DragFloat3("SphereRotate",
                        reinterpret_cast<float *>(&transform.rotate), 0.01f);
      ImGui::DragFloat3("SphereScale",
                        reinterpret_cast<float *>(&transform.scale), 0.01f,
                        0.0f, 5.0f);

      // 平面
      ImGui::Text("Plane");

      ImGui::DragFloat3("PlaneTranslate", &transform2.translate.x, 0.01f);
      ImGui::DragFloat3("PlaneRotate", &transform2.rotate.x, 0.01f);
      ImGui::DragFloat3("PlaneScale", &transform2.scale.x, 0.01f, 0.0f, 5.0f);

      // Sprite
      ImGui::Text("Sprite");

      ImGui::DragFloat3("SpriteTranslate", &transformSprite.translate.x, 1.0f);
      ImGui::DragFloat3("SpriteRotate", &transformSprite.rotate.x, 0.01f);
      ImGui::DragFloat3("SpriteScale", &transformSprite.scale.x, 0.01f);

      // UV
      ImGui::Text("UV");

      ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f,
                        -10.0f, 10.0f);
      ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
                        10.0f);
      ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

      // サウンド
      ImGui::Text("Sound");
      ImGui::SliderFloat("Volume", &selectVol, 0.0f, 1.0f);
      if (ImGui::Button("Play")) {
        audio.Play("select", /*loop=*/false, selectVol);
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        audio.Stop("select");
      }

      ImGui::End();
    }

    ImGui::Begin("Particle");
    static int particleIndex = 0;
    ImGui::SliderInt("Index", &particleIndex, 0, kParticleCount - 1);

    ImGui::DragFloat3("ParticleTranslate",
                      &particles[particleIndex].transform.translate.x, 0.01f);
    ImGui::DragFloat3("ParticleRotate",
                      &particles[particleIndex].transform.rotate.x, 0.01f);
    ImGui::DragFloat3("ParticleScale",
                      &particles[particleIndex].transform.scale.x, 0.01f, 0.0f,
                      5.0f);
    ImGui::End();

    // ImGuiの内部コマンドを生成する
    ImGui::Render();

    input.Update();

    // 既存のデバッグ出力
    if (input.WasPressed(DIK_0) || input.WasMousePressed(0) ||
        input.WasPadPressed(XINPUT_GAMEPAD_A)) {
      OutputDebugStringA("Hit 0\n");
    }

    // カメラ更新も置き換え
    debugCamera->Update(input);

    Matrix4x4 cameraMat =
        MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, // 回転なし
                         {0.0f, 0.0f, -10.0f});

    // カメラをキー入力ありの物に
    Matrix4x4 viewMatrix = Inverse(cameraMat);

    Matrix4x4 projectionMatrix =
        MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

    Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate,
                                             transform.translate);

    // モデル側へ反映（CB更新はモデル内部）
    model.SetWorldTransform(worldMatrix);

    // カメラをキー入力ありの物に
    Matrix4x4 viewMatrix2 = debugCamera->GetViewMatrix();

    Matrix4x4 projectionMatrix2 =
        MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

    Matrix4x4 planeWorld = MakeAffineMatrix(transform2.scale, transform2.rotate,
                                            transform2.translate);
    planeModel.SetWorldTransform(planeWorld);

    model.SetLightingMode(lightingMode);

    // === Particle 行列更新 ===
    for (uint32_t i = 0; i < kParticleInstanceCount; ++i) {
      // 位置は既存 particles[i].transform を使う（上で y を揺らしている）
      Matrix4x4 worldP = MakeAffineMatrix(particles[i].transform.scale,
                                          particles[i].transform.rotate,
                                          particles[i].transform.translate);
      Matrix4x4 wvpP = Multiply(worldP, Multiply(viewMatrix, projectionMatrix));
      particleMatrices[i].World = worldP;
      particleMatrices[i].WVP = wvpP;
    }

    /* WVPMatrixを作って書き込む
    ---------------------------------*/
    // スプライトの位置・スケール・回転などを反映
    sprite.SetPosition(transformSprite.translate);
    sprite.SetScale(transformSprite.scale);
    sprite.SetRotation(transformSprite.rotate);

    // UV 変換も反映
    Matrix4x4 uvMat =
        Multiply(Multiply(MakeScaleMatrix(uvTransformSprite.scale),
                          MakeRotateZMatrix(uvTransformSprite.rotate.z)),
                 MakeTranslateMatrix(uvTransformSprite.translate));
    sprite.SetUVTransform(uvMat);

    /*for (uint32_t i = 0; i < kNumInstance; ++i) {
      Matrix4x4 worldMatrix = MakeAffineMatrix(
          transforms[i].scale, transforms[i].rotate, transforms[i].translate);
      Matrix4x4 wvpMatrix = Multiply(worldMatrix, viewMatrix);
      instancingData[i].World = worldMatrix;
      instancingData[i].WVP = wvpMatrix;
    }*/

    // ゲームの処理終わり

    // 描画先のRTVとDSVを設定する
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dx.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

    // 指定した色で画面全体をクリアする
    float clearColor[] = {
        0.1f,
        0.25f,
        0.5f,
        1.0f,
    }; // 青っぽい色。 RGBAの順

    /*コマンドを積み込んで確定させる
    ------------------------------*/
    // これから書き込むバックバッファのインデクスを取得
    UINT backBufferIndex = dx.GetSwapChain()->GetCurrentBackBufferIndex();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        dx.GetRTVHandle(backBufferIndex); // ← 左辺値にする
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0,
                                   nullptr); // ← これも同じ変数を使ってOK
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
                                   0, nullptr);

    /*コマンドを積む
    -----------------------*/
    ID3D12GraphicsCommandList *cl = cmdList.Get();

    cl->RSSetViewports(1, &viewport);       // viewportを設定
    cl->RSSetScissorRects(1, &scissorRect); // Scissorを設定

    // モデル描画（内部で RS/PSO/CBV/SRV/VB/IB をまとめてセット）
    cl->SetGraphicsRootSignature(objPipeline.GetRootSignature());
    cl->SetPipelineState(objPipeline.GetPipelineState());
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //cmdList->SetGraphicsRootDescriptorTable(
    //    2, instancingSrvHandleGPU); // インスタンス行列群のSRVをセット

    //for (uint32_t i = 0; i < kNumInstance; ++i) {
    //  cmdList->SetGraphicsRootConstantBufferView(
    //      0, instancingResource->GetGPUVirtualAddress() +
    //             sizeof(TransformationMatrix) * i);
    //}

    // === Particle Instanced Draw ===
    {
      ID3D12GraphicsCommandList *clp = cmdList.Get();
      clp->SetGraphicsRootSignature(particlePipeline.GetRootSignature());
      clp->SetPipelineState(particlePipeline.GetPipelineState());
      clp->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      // ヒープ設定（テクスチャと行列 SRV は同一ヒープ）
      ID3D12DescriptorHeap *heaps[] = {dx.GetSRVHeap()};
      clp->SetDescriptorHeaps(1, heaps);

      // Material (PS:b0) / Texture (PS:t0) / Instancing (VS:t1)
      // planeModel のリソースを再利用（ゲッタ追加前提）
      clp->SetGraphicsRootConstantBufferView(
          0, planeModel.GetMaterialCB()->GetGPUVirtualAddress());
      clp->SetGraphicsRootDescriptorTable(1, planeModel.GetTextureHandle());
      clp->SetGraphicsRootDescriptorTable(2, particleMatricesSrvGPU); // ★ t1

      // 頂点バッファ（planeModel の VB）
      auto vbv = planeModel.GetVBV();
      clp->IASetVertexBuffers(0, 1, &vbv);

      // 今の Model 実装はインデックス無し → 頂点数 = planeModel 頂点数
      UINT vertexCount = vbv.SizeInBytes / vbv.StrideInBytes;
      clp->DrawInstanced(vertexCount, kParticleInstanceCount, 0, 0);
    }

    /*model.Draw(viewMatrix, projectionMatrix,
      directionalLightResource.Get());
    planeModel.Draw(viewMatrix, projectionMatrix,
                    directionalLightResource.Get());*/

    //cmdList->DrawInstanced(UINT(modelData.vertices.size()), kNumInstance, 0, 0);

    /* 2D描画コマンドを積む
    -----------------------------*/
    Matrix4x4 view = MakeIdentity4x4();
    Matrix4x4 proj = MakeOrthographicMatrix(
        0.0f, 0.0f, static_cast<float>(app.kClientWidth),
        static_cast<float>(app.kClientHeight), 0.0f, 1.0f);

    // ===== Sprite の BlendMode を選択 =====
    UnifiedPipeline *currentSpritePipeline = &spritePipelineAlpha;
    switch (spriteBlendMode) {
    case 0:
      currentSpritePipeline = &spritePipelineAlpha;
      break; // Alpha
    case 1:
      currentSpritePipeline = &spritePipelineAdd;
      break; // Add
    case 2:
      currentSpritePipeline = &spritePipelineSub;
      break; // Sub
    case 3:
      currentSpritePipeline = &spritePipelineMul;
      break; // Mul
    case 4:
      currentSpritePipeline = &spritePipelineScreen;
      break; // Screen
    }

    sprite.SetPipeline(currentSpritePipeline);

    // sprite.Draw(view, proj);

    /* ImGuiを描画
    -------------------------------*/
    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

    dx.EndFrame();
  }

  /*解放処理
  --------------------*/
  // 音声データ開放
  audio.Shutdown();
  delete debugCamera;
  input.Finalize();
  app.Finalize();

  CoUninitialize();

  return 0;
}

IDxcBlob *CompileShader(
    // CompilerするShaderファイルへのパス
    const std::wstring &filePath,
    // Compilerに使用するProfile
    const wchar_t *profile,
    // 初期化で生成したものを三つ
    IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
    IDxcIncludeHandler *includeHandler) {

  /*hlslファイルを読む
  --------------------------*/
  // これからシェーダーをコンパイルする旨をログに出す
  Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n",
                                filePath, profile)));
  // hlslファイルを読む
  IDxcBlobEncoding *shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
  // 読み込めなかったら止める
  assert(SUCCEEDED(hr));
  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

  /*Compileする
  ---------------------------*/
  LPCWSTR arguments[] = {
      filePath.c_str(), // コンパイル対象の、hlslファイル名
      L"-E",
      L"main", // エントリーポイントの指定。基本的に、main以外にはしない
      L"-T",
      profile, // ShaderProfileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"-Od",           // 最適化を外しておく
      L"-Zpr",          // メモリレイアウトは行優先
  };

  // 実際にShaderをコンパイルする
  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer, // 読み込んだファイル
                            arguments,           // コンパイルオプション
                            _countof(arguments), // コンパイルオプションの数
                            includeHandler,      // includeが含まれた諸々
                            IID_PPV_ARGS(&shaderResult) // コンパイル結果
  );
  // コンパイルエラーではなく、dxcが起動できないなど致命的な状況
  if (FAILED(hr)) {
    OutputDebugStringA("テクスチャのリソース作成に失敗しました\n");
    return nullptr; // or return エラーハンドリングへ
  }
  assert(SUCCEEDED(hr));

  /*警告・エラーが出ていないか確認する
  -----------------------------------*/
  // 警告・エラーが出ていたらログに出して止める
  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());
    // 警告・エラーダメ、絶対
    assert(false);
  }

  /* Compileを受け取って返す
  ------------------------------*/
  // コンパイル結果から実行用のバイナリ部分を取得
  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));
  // 成功したログを出す
  Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n",
                                filePath, profile)));
  // もう使わないリソースを解散
  shaderSource->Release();
  shaderResult->Release();
  // 実行用のバイナリを返却
  return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                     size_t sizeInBytes) {
  // 頂点リソースのヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
  // 頂点リソースの設定
  D3D12_RESOURCE_DESC vertexResourceDesc{};
  // バッファのリソース。テクスチャの場合はまた別の設定をする
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width =
      sizeInBytes; // リソースのサイズ。今回はVector4を3頂点分
  // バッファの場合はこれらは1にする決まり
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;
  // バッファ用の場合はこれにする決まり
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;

  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vertexResource));
  assert(SUCCEEDED(hr));
  return vertexResource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                     D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                     bool shaderVisible) {
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
  descriptorHeapDesc.NumDescriptors =
      numDescriptors; // ダブルバッファ用に2つ。多くても別にかまわない
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc,
                                            IID_PPV_ARGS(&descriptorHeap));
  // ディスクリプタヒープが作れなかったので起動できない
  assert(SUCCEEDED(hr));
  return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                      const DirectX::TexMetadata &metadata) {
  // 1. metadataを基にResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
  resourceDesc.DepthOrArraySize =
      UINT16(metadata.arraySize);        // 奥行き or 配列数
  resourceDesc.Format = metadata.format; // Format
  resourceDesc.SampleDesc.Count = 1;     // サンプリング数。1固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(
      metadata.dimension); // Textureの次元数。普段使っているのは2次元

  // 2. 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
  heapProperties.CPUPageProperty =
      D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
  heapProperties.MemoryPoolPreference =
      D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

  // 3. Resourceを生成する
  Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                   // Heapの設定
      D3D12_HEAP_FLAG_NONE,              // Heapの特殊設定。
      &resourceDesc,                     // Resourceの設定
      D3D12_RESOURCE_STATE_GENERIC_READ, // 読み取り用に設定
      nullptr,                           // Clear値は使わない
      IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr)); // デバッグ時に失敗しないことを保証

  return resource;
}

void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                       const DirectX::ScratchImage &mipImages) {
  // meta情報を取得
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  // 全MipMapについて
  for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
    // MipMapLevelを指定して各Imageを取得
    const DirectX::Image *img = mipImages.GetImage(mipLevel, 0, 0);
    // Textureに転送
    HRESULT hr =
        texture->WriteToSubresource(UINT(mipLevel), nullptr, // 全領域へコピー
                                    img->pixels,             // 元データアドレス
                                    UINT(img->rowPitch),     // 1ラインサイズ
                                    UINT(img->slicePitch)    // 1枚サイズ
        );
    assert(SUCCEEDED(hr));
  }
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(
    const Microsoft::WRL::ComPtr<ID3D12Device> &device, INT32 width,
    INT32 height) {
  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = width;        // Textureの幅
  resourceDesc.Height = height;      // Textureの高さ
  resourceDesc.MipLevels = 1;        // mipMapの数
  resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
  resourceDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定。
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
  resourceDesc.Flags =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

  // 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

  // 深度値のクリア設定
  D3D12_CLEAR_VALUE depthClearValue{};
  depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f（最大値）でクリア
  depthClearValue.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

  // Resourceの生成
  Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                  // Heapの設定
      D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定。
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
      &depthClearValue,                 // Clear最適地
      IID_PPV_ARGS(&resource)           // 作成するResourceポインタへのポインタ
  );
  assert(SUCCEEDED(hr));
  return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (descriptorSize * index);
  return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (descriptorSize * index);
  return handleGPU;
}

ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename) {
  // 1.中で必要となる変数の宣言
  ModelData modelData;            // 構築するModelData
  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んだ1行を格納するもの

  // 2.ファイルを開く
  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  // 3.実際にファイルを読み、ModelDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    // identifierに応じた処理
    if (identifier == "v") {
      Vector4 position;
      s >> position.x >> position.y >> position.z;
      position.w = 1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (identifier == "f") {
      VertexData triangle[3];
      // 面は三角形限定。その他は未対応。
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        // 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
        std::istringstream v(vertexDefinition);
        uint32_t elementIndices[3];
        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
          elementIndices[element] = std::stoi(index);
        }
        // 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];
        /*VertexData vertex = {position, texcoord, normal};
        modelData.vertices.push_back(vertex);*/

        position.x *= -1.0f;
        normal.x *= -1.0f;
        texcoord.y = 1.0f - texcoord.y;

        triangle[faceVertex] = {position, texcoord, normal};
      }
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      // materialTemplateLibraryファイルの名前を取得する
      std::string materialFilename;
      s >> materialFilename;
      // 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }

  // 4.ModelDataを返す
  return modelData;
}

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename) {
  // 1.中で必要となる変数の宣言
  MaterialData materialData; // 構築するMaterialData
  std::string line;          // ファイルから読んだ1行を格納するもの

  // 2.ファイルを開く
  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  // 3.実際にファイルを読み、ModelDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier;

    // identifierに応じた処理
    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;
      // 連結してファイルパスにする
      materialData.textureFilePath = directoryPath + "/" + textureFilename;
    }
  }

  // 4.MaterialDataを返す
  return materialData;
}