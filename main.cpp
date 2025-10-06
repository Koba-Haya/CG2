#include "Audio.h"
#include "DirectXCommon.h"
#include "UnifiedPipeline.h"
#include "WinApp.h"
#include "include.h"
#include <Windows.h>

struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
  Vector3 normal;
};

struct Material {
  Vector4 color;
  int32_t enableLighting;
  float padding[3];
  Matrix4x4 uvTransform;
};

struct TransformationMatrix {
  Matrix4x4 WVP;
  Matrix4x4 World;
};

struct DirectionalLight {
  Vector4 color;     // ライトの色
  Vector3 direction; // ライトの向き
  float intensity;   // 輝度
};

struct MaterialData {
  std::string textureFilePath;
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
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

std::wstring ConvertString(const std::string &str);

std::string ConvertString(const std::wstring &str);

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

// ========================================================
// SRV割り当ての小ユーティリティ（ImGui が 0 を使用）
// ========================================================
struct SrvAllocator {
  ID3D12Device *device = nullptr;
  ID3D12DescriptorHeap *heap = nullptr;
  UINT inc = 0;
  UINT next = 1; // 0 は ImGui 用とする

  void Init(ID3D12Device *dev, ID3D12DescriptorHeap *h) {
    device = dev;
    heap = h;
    inc = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    next = 1;
  }
  UINT Allocate() { return next++; }

  D3D12_CPU_DESCRIPTOR_HANDLE Cpu(UINT index) const {
    return GetCPUDescriptorHandle(heap, inc, index);
  }
  D3D12_GPU_DESCRIPTOR_HANDLE Gpu(UINT index) const {
    return GetGPUDescriptorHandle(heap, inc, index);
  }
};

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
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx.GetCommandList();
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap =
      dx.GetSRVHeap();
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = dx.GetSwapChain();

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

  // ウィンドウを表示する
  // ShowWindow(hwnd, SW_SHOW);

  MSG msg{};

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
  auto objDesc =
      UnifiedPipeline::MakeObject3DDesc();
  bool ok = objPipeline.Initialize(device.Get(), dxcUtils, dxcCompiler,
                                   includeHandler, objDesc);
  assert(ok);

  UnifiedPipeline spritePipeline;
  auto sprDesc =
      UnifiedPipeline::MakeSpriteDesc(); // Sprite.VS/PS.hlslがある場合
  sprDesc.vsPath = L"Object3D.VS.hlsl";
  sprDesc.psPath = L"Object3D.PS.hlsl";
  bool ok2 = spritePipeline.Initialize(device.Get(), dxcUtils, dxcCompiler,
                                       includeHandler, sprDesc);
  assert(ok2);

  // 関数に丸投げ
  /*Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
  CreateBasicPSO(device.Get(), dxcUtils, dxcCompiler, includeHandler,
                 rootSignature, graphicsPipelineState);*/

  auto rootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>(
      objPipeline.GetRootSignature());
  auto graphicsPipelineState = Microsoft::WRL::ComPtr<ID3D12PipelineState>(
      objPipeline.GetPipelineState());

  // モデル読み込み
  ModelData modelData = LoadObjFile("resources", "sphere.obj");
  // 頂点リソースを作る
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(
      device.Get(), sizeof(VertexData) * modelData.vertices.size());
  // 頂点バッファービューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  vertexBufferView.BufferLocation =
      vertexResource
          ->GetGPUVirtualAddress(); // リソースの先頭のアドレスから使う
  vertexBufferView.SizeInBytes =
      UINT(sizeof(VertexData) *
           modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点あたりのサイズ

  // 頂点リソースにデータを書き込む
  VertexData *vertexData = nullptr;
  vertexResource->Map(
      0, nullptr,
      reinterpret_cast<void **>(&vertexData)); // 書き込むためのアドレスを取得
  std::memcpy(vertexData, modelData.vertices.data(),
              sizeof(VertexData) *
                  modelData.vertices.size()); // 頂点データをリソースにコピー

  // モデル二つ目
  ModelData modelData2 = LoadObjFile("resources", "plane.obj");
  // 頂点リソースを作る
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource2 = CreateBufferResource(
      device.Get(), sizeof(VertexData) * modelData2.vertices.size());

  // 頂点バッファービューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView2{};
  vertexBufferView2.BufferLocation =
      vertexResource2
          ->GetGPUVirtualAddress(); // リソースの先頭のアドレスから使う
  vertexBufferView2.SizeInBytes = UINT(
      sizeof(VertexData) *
      modelData2.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
  vertexBufferView2.StrideInBytes = sizeof(VertexData); // 1頂点あたりのサイズ

  // 頂点リソースにデータを書き込む
  VertexData *vertexData2 = nullptr;
  vertexResource2->Map(
      0, nullptr,
      reinterpret_cast<void **>(&vertexData2)); // 書き込むためのアドレスを取得
  std::memcpy(vertexData2, modelData2.vertices.data(),
              sizeof(VertexData) *
                  modelData2.vertices.size()); // 頂点データをリソースにコピー

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

  /* MaterialようのResourceを作る
  -----------------------------------*/
  // マテリアル用のリソースを作る。今回は、color1つ分のサイズを用意する
  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =
      CreateBufferResource(device.Get(), sizeof(Material));
  // マテリアルにデータを書き込む
  Material *materialData = nullptr;
  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  // 今回は赤を書き込んでみる
  *materialData = {Vector4(1.0f, 1.0f, 1.0f, 1.0f), true};
  materialData->uvTransform = MakeIdentity4x4();

  /* TransformationMatrix用のResourceを作る
  -----------------------------------------------------*/
  UINT transformationMatrixSize = (sizeof(TransformationMatrix) + 255) & ~255;
  assert(transformationMatrixSize / 256 == 1);
  // WVP用のリソースを作る。Matrix4x4 2つ分のサイズを用意する
  Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource =
      CreateBufferResource(device.Get(), transformationMatrixSize);
  // データを描き込む
  TransformationMatrix *wvpData = nullptr;
  // 書き込むためのアドレスを取得
  wvpResource->Map(0, nullptr, reinterpret_cast<void **>(&wvpData));
  // 単位行列に書き込んでおく
  *wvpData = {MakeIdentity4x4(), MakeIdentity4x4()};

  Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource2 =
      CreateBufferResource(device.Get(), transformationMatrixSize);
  TransformationMatrix *wvpData2 = nullptr;
  wvpResource2->Map(0, nullptr, reinterpret_cast<void **>(&wvpData2));
  *wvpData2 = {MakeIdentity4x4(), MakeIdentity4x4()};

  // Textureを読んで転送する
  DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource =
      CreateTextureResource(device.Get(), metadata);
  UploadTextureData(textureResource.Get(), mipImages);

  // 2枚目のTextureを読んで転送する
  DirectX::ScratchImage mipImage2 =
      LoadTexture(modelData.material.textureFilePath);
  const DirectX::TexMetadata &metadata2 = mipImage2.GetMetadata();
  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 =
      CreateTextureResource(device.Get(), metadata2);
  UploadTextureData(textureResource2.Get(), mipImage2);

  /* ShaderResourceViewを作る
  ---------------------------------------*/
  // metadataを基にSRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  // metadataを基にSRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
  srvDesc2.Format = metadata2.format;
  srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

  // 1回だけ初期化（srvDescriptorHeap が使えるスコープで）
  SrvAllocator srvAlloc;
  srvAlloc.Init(device.Get(), srvDescriptorHeap.Get());

  // --- 1枚目のテクスチャ ---
  UINT texIdx1 = srvAlloc.Allocate();
  device->CreateShaderResourceView(textureResource.Get(), &srvDesc,
                                   srvAlloc.Cpu(texIdx1));
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvAlloc.Gpu(texIdx1);

  // --- 2枚目のテクスチャ ---
  UINT texIdx2 = srvAlloc.Allocate();
  device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2,
                                   srvAlloc.Cpu(texIdx2));
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = srvAlloc.Gpu(texIdx2);

  /* VertexResourceとVertexBufferViewを用意
  ---------------------------------------------------*/
  // Sprite用の頂点リソースを作る
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite =
      CreateBufferResource(device.Get(), sizeof(VertexData) * 6);
  // 頂点バッファービューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
  // リソースの先頭のアドレスから使う
  vertexBufferViewSprite.BufferLocation =
      vertexResourceSprite->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点６つ分のサイズ
  vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
  // １頂点当たりのサイズ
  vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

  /* 頂点データを設定する
--------------------------------------------*/
  VertexData *vertexDataSprite = nullptr;
  vertexResourceSprite->Map(0, nullptr,
                            reinterpret_cast<void **>(&vertexDataSprite));
  // 1枚目の三角形
  vertexDataSprite[0].position = {0.0f, 360.0f, 0.0f, 1.0f}; // 左下
  vertexDataSprite[0].texcoord = {0.0f, 1.0f};
  vertexDataSprite[0].normal = {0.0f, 0.0f, -1.0f};
  vertexDataSprite[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexDataSprite[1].texcoord = {0.0f, 0.0f};
  vertexDataSprite[1].normal = {0.0f, 0.0f, -1.0f};
  vertexDataSprite[2].position = {640.0f, 360.0f, 0.0f, 1.0f}; // 右下
  vertexDataSprite[2].texcoord = {1.0f, 1.0f};
  vertexDataSprite[2].normal = {0.0f, 0.0f, -1.0f};
  // ２枚目の三角形
  vertexDataSprite[3].position = {640.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexDataSprite[3].texcoord = {1.0f, 0.0f};
  vertexDataSprite[3].normal = {0.0f, 0.0f, 1.0f};

  /* Transform周りをつくる
-------------------------------------*/
  // Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4
  // 1つ分のサイズを用意する。
  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite =
      CreateBufferResource(device.Get(), sizeof(Matrix4x4) + 64);
  // データを書き込む
  Matrix4x4 *transformationMatrixDataSprite = nullptr;
  // 書き込むためのアドレスを取得
  transformationMatrixResourceSprite->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));
  // 単位行列を書き込んでおく
  *transformationMatrixDataSprite = MakeIdentity4x4();

  /* Sprite用のマテリアルを作成して利用する
  --------------------------------------------*/
  // Sprite用のマテリアルリソースを作る
  Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite =
      CreateBufferResource(device.Get(), sizeof(Material));
  // マテリアルにデータを書き込む
  Material *materialDataSprite = nullptr;
  // 書き込むためのアドレスを取得
  materialResourceSprite->Map(0, nullptr,
                              reinterpret_cast<void **>(&materialDataSprite));
  // 今回は白を書き込んでみる
  // SpriteはLightingしないのでfalseを設定する
  *materialDataSprite = {Vector4(1.0f, 1.0f, 1.0f, 0.0f), 0};
  materialDataSprite->uvTransform = MakeIdentity4x4();

  /* 平行光源をShaderで使う
  --------------------------------------*/
  Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
      CreateBufferResource(device.Get(), sizeof(DirectionalLight));
  DirectionalLight *directionalLightData = nullptr;
  // デフォルト値はとりあえず以下のようにしておく
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));
  *directionalLightData = {{1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 1.0f};

  /* index用のあれやこれや
  ----------------------------------*/
  // Resource作成
  Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite =
      CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);
  // View作成
  D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
  // リソースの先頭のアドレスから使う
  indexBufferViewSprite.BufferLocation =
      indexResourceSprite->GetGPUVirtualAddress();
  // 使用するリソースのサイズはインデックス6つ分のサイズ
  indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
  // インデックスはuint32_tとする
  indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

  /* IndexResourceにデータを書き込む
  -----------------------------------*/
  uint32_t *indexDataSprite = nullptr;
  indexResourceSprite->Map(0, nullptr,
                           reinterpret_cast<void **>(&indexDataSprite));
  indexDataSprite[0] = 0;
  indexDataSprite[1] = 1;
  indexDataSprite[2] = 2;
  indexDataSprite[3] = 1;
  indexDataSprite[4] = 3;
  indexDataSprite[5] = 2;

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

  bool useMonsterBall = true;

  static int lightingMode = 1; // 初期値: Lambert

  // 音声読み込み
  bool select =
      audio.Load("select", L"resources/select.mp3", 1.0f); // mp3等もOK
  assert(select);
  static float selectVol = 1.0f;

  DebugCamera *debugCamera = new DebugCamera();

  // ImGuiの初期化

  // ウィンドウの×ボタンが押されるまでループ
  while (app.ProcessMessage()) {

    // ゲームの処理
    dx.BeginFrame();

    ID3D12DescriptorHeap *heaps[] = {srvDescriptorHeap.Get()};
    commandList->SetDescriptorHeaps(1, heaps);

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

      // マテリアルの色
      ImGui::ColorEdit3("Color",
                        reinterpret_cast<float *>(&materialData->color));

      // Lighting切り替え
      ImGui::Text("Lighting Mode");
      ImGui::RadioButton("None", &lightingMode, 0);
      ImGui::RadioButton("Lambert", &lightingMode, 1);
      ImGui::RadioButton("Half-Lambert", &lightingMode, 2);

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

    Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate,
                                             transform.translate);
    Matrix4x4 cameraMatrix =
        MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
                         cameraTransform.translate);

    // カメラをキー入力ありの物に
    Matrix4x4 viewMatrix = debugCamera->GetViewMatrix();

    Matrix4x4 projectionMatrix =
        MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
    Matrix4x4 worldViewProjectionMatrix =
        Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
    *wvpData = {worldViewProjectionMatrix, worldMatrix};

    Matrix4x4 worldMatrix2 = MakeAffineMatrix(
        transform2.scale, transform2.rotate, transform2.translate);

    // カメラをキー入力ありの物に
    Matrix4x4 viewMatrix2 = debugCamera->GetViewMatrix();

    Matrix4x4 projectionMatrix2 =
        MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
    Matrix4x4 worldViewProjectionMatrix2 =
        Multiply(worldMatrix2, Multiply(viewMatrix2, projectionMatrix2));
    *wvpData2 = {worldViewProjectionMatrix2, worldMatrix2};

    /* WVPMatrixを作って書き込む
    ---------------------------------*/
    // Sprite用のWorldViewProjectionMatrixを作る
    Matrix4x4 worldMatrixSprite =
        MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
                         transformSprite.translate);
    Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
    Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
        0.0f, 0.0f, static_cast<float>(app.kClientWidth),
        static_cast<float>(app.kClientHeight), 0.0f, 100.0f);
    Matrix4x4 worldViewProjectionMatrixSprite = Multiply(
        worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
    *transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

    Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
    uvTransformMatrix = Multiply(
        uvTransformMatrix, (MakeRotateZMatrix(uvTransformSprite.rotate.z)));
    uvTransformMatrix = Multiply(
        uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
    materialDataSprite->uvTransform = uvTransformMatrix;

    materialData->enableLighting = lightingMode;

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
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0,
                                       nullptr); // ← これも同じ変数を使ってOK
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f,
                                       0, 0, nullptr);

    /*コマンドを積む
    -----------------------*/
    commandList->RSSetViewports(1, &viewport);       // viewportを設定
    commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定
    // RootSignatureを設定。PSOに設定しているけど別途設定が必要
    commandList->SetGraphicsRootSignature(objPipeline.GetRootSignature());
    commandList->SetPipelineState(objPipeline.GetPipelineState()); // PSOを設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);   // VBVを設定
    // commandList->IASetIndexBuffer(&indexBufferView);          // IBVを設定
    //  形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    /*CBVを設定する
    --------------------------------------------------------*/
    // マテリアルCBufferの場所を特定
    commandList->SetGraphicsRootConstantBufferView(
        0, materialResource->GetGPUVirtualAddress());
    // wvp用のCBufferの場所を特定
    commandList->SetGraphicsRootConstantBufferView(
        1, wvpResource->GetGPUVirtualAddress());
    /* DescriptorTableを設定する
    --------------------------------*/
    // SRVのDescriptorTableの先頭を設定。2は、rootParamater[2]である。
    commandList->SetGraphicsRootDescriptorTable(
        2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
    commandList->SetGraphicsRootConstantBufferView(
        3, directionalLightResource->GetGPUVirtualAddress());
    /*------------------------------------------------------*/
    // 描画 (DrawCall/ドローコール)。6頂点で1つのインスタンス。
    // commandList->DrawIndexedInstanced(kSubdivision * kSubdivision * 6, 1, 0,
    // 0,0);
    commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

    // 頂点バッファ変更
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView2);

    // マテリアル設定（既存のmaterialResourceでOKならそのまま）
    commandList->SetGraphicsRootConstantBufferView(
        0, materialResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(
        1, wvpResource2->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(
        2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
    commandList->SetGraphicsRootConstantBufferView(
        3, directionalLightResource->GetGPUVirtualAddress());

    // 描画
    commandList->DrawInstanced(UINT(modelData2.vertices.size()), 1, 0, 0);

    /* 2D描画コマンドを積む
    -----------------------------*/
    commandList->SetGraphicsRootSignature(spritePipeline.GetRootSignature());
    commandList->SetPipelineState(spritePipeline.GetPipelineState());
    // Spriteの描画。変更が必要なものだけ変更する。
    commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite); // VBVを設定
    commandList->IASetIndexBuffer(&indexBufferViewSprite); // IBVを設定
    // TransformationMatrixCBufferの場所を設定
    commandList->SetGraphicsRootConstantBufferView(
        1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
    // マテリアルCBufferの場所を特定
    commandList->SetGraphicsRootConstantBufferView(
        0, materialResourceSprite->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
    // 描画（DrawCall/ドローコール）
    // commandList->DrawInstanced(6, 1, 0, 0);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    /* ImGuiを描画
    -------------------------------*/
    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

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

std::wstring ConvertString(const std::string &str) {
  if (str.empty()) {
    return std::wstring();
  }

  auto sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                          static_cast<int>(str.size()), NULL, 0);
  if (sizeNeeded == 0) {
    return std::wstring();
  }
  std::wstring result(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                      static_cast<int>(str.size()), &result[0], sizeNeeded);
  return result;
}

std::string ConvertString(const std::wstring &str) {
  if (str.empty()) {
    return std::string();
  }

  auto sizeNeeded =
      WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                          NULL, 0, NULL, NULL);
  if (sizeNeeded == 0) {
    return std::string();
  }
  std::string result(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      result.data(), sizeNeeded, NULL, NULL);
  return result;
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
                            arguments, // コンパイルオプション
                            _countof(arguments), // コンパイルオプションの数
                            includeHandler, // includeが含まれた諸々
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

DirectX::ScratchImage LoadTexture(const std::string &filePath) {
  // テクスチャファイルを読み込んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(filePath);

  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミップマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // ミップマップ付きのデータを返す
  return mipImages;
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
                                    img->pixels, // 元データアドレス
                                    UINT(img->rowPitch),  // 1ラインサイズ
                                    UINT(img->slicePitch) // 1枚サイズ
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
      IID_PPV_ARGS(&resource) // 作成するResourceポインタへのポインタ
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
  std::string line; // ファイルから読んだ1行を格納するもの

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
  std::string line; // ファイルから読んだ1行を格納するもの

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
