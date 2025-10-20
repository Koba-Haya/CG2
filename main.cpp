#include "EngineCore.h"
#include "Model.h"
#include "Sprite.h"
#include "UnifiedPipeline.h"
#include "include.h"
#include <Windows.h>

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

/* Textureデータを読む
-----------------------------*/
DirectX::ScratchImage LoadTexture(const std::string &filePath);

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

  // ウィンドウ初期化
  MSG msg{};

  /* ReportLiveObjects
  ----------------------------*/
  D3DResourceLeakChecker leakCheck;

  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  assert(SUCCEEDED(hr));

  EngineCore engine;
  engine.Initialize();

  auto *dx = engine.GetDX();
  auto *input = engine.GetInput();
  auto *rm = engine.GetResourceManager();
  auto *shaderCompiler = engine.GetShaderCompiler();
  auto *assets = engine.GetAssets();
  auto *audio = engine.GetAudio();
  auto *app = engine.GetWinApp();

  Microsoft::WRL::ComPtr<ID3D12Device> device = dx->GetDevice();

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

  //================================
  //              PSO
  //================================

  // DXC は DirectXCommon から借りる
  auto *dxcUtils = dx->GetDXCUtils();
  auto *dxcCompiler = dx->GetDXCCompiler();
  auto *includeHandler = dx->GetDXCIncludeHandler();

  //================================
  //              モデル
  //================================

  // 各種パイプラインやモデル初期化
  UnifiedPipeline objPipeline;
  objPipeline.Initialize(dx->GetDevice(), shaderCompiler,
                         UnifiedPipeline::MakeObject3DDesc());

  Model model;
  Model::CreateInfo ci{};
  ci.dx = dx;
  ci.pipeline = &objPipeline;
  ci.resourceManager = engine.GetResourceManager();
  ci.modelData = assets->LoadObj("resources", "sphere.obj");
  ci.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  ci.lightingMode = 1; // 0:Unlit 1:Lambert 2:HalfLambert

  bool okModel = model.Initialize(ci);
  assert(okModel);

  Model planeModel;
  Model::CreateInfo ci2{};
  ci2.dx = dx;
  ci2.pipeline = &objPipeline;
  ci2.resourceManager = engine.GetResourceManager();
  ci2.modelData = assets->LoadObj("resources", "plane.obj");
  ci2.baseColor = {1.0f, 1.0f, 1.0f, 1.0f}; // 色だけ変えるなど
  ci2.lightingMode = 1;

  bool okModel2 = planeModel.Initialize(ci2);
  assert(okModel2);

  //================================
  //            スプライト
  //================================

  UnifiedPipeline spritePipeline;
  spritePipeline.Initialize(dx->GetDevice(), shaderCompiler,
                            UnifiedPipeline::MakeSpriteDesc());
  // Sprite 用オブジェクト
  Sprite::CreateInfo sprInfo{};
  sprInfo.dx = dx;
  sprInfo.pipeline = &spritePipeline; // ✅ Spriteパイプラインを渡す！
  sprInfo.resourceManager = engine.GetResourceManager();
  sprInfo.texturePath = "resources/uvChecker.png";
  sprInfo.size = {640.0f, 360.0f};
  sprInfo.color = {1.0f, 1.0f, 1.0f, 1.0f};
  Sprite sprite;

  bool okSprite = sprite.Initialize(sprInfo);
  assert(okSprite);

  // ===================== 3Dオブジェクト用 =====================
  auto rootSignature3D = objPipeline.GetRootSignature();
  auto pso3D = objPipeline.GetPipelineState();

  // ===================== スプライト用 =====================
  auto rootSignature2D = spritePipeline.GetRootSignature();
  auto pso2D = spritePipeline.GetPipelineState();

  /*ViewportとScissor
  -------------------------*/
  // ビューポート
  D3D12_VIEWPORT viewport{};
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = app->kClientWidth;
  viewport.Height = app->kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  // シザー矩形
  D3D12_RECT scissorRect{};
  // 基本的にビューポートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = app->kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = app->kClientHeight;

  /* 平行光源をShaderで使う
  --------------------------------------*/
  // どこか初期化直後に（PS:b1用）
  auto directionalLightResource =
      rm->CreateUploadBuffer(sizeof(DirectionalLight));
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

  bool useMonsterBall = true;

  static int lightingMode = 1; // 初期値: Lambert

  // 音声読み込み
  bool select =
      audio->Load("select", L"resources/select.mp3", 1.0f); // mp3等もOK
  assert(select);
  static float selectVol = 1.0f;

  DebugCamera *debugCamera = new DebugCamera();

  // ImGuiの初期化

  // ウィンドウの×ボタンが押されるまでループ
  while (engine.GetWinApp()->ProcessMessage()) {

    // ゲームの処理
    engine.BeginFrame();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList =
        dx->GetCommandList();

    ID3D12DescriptorHeap *heaps[] = {engine.GetResourceManager()->GetSRVHeap()};
    cmdList->SetDescriptorHeaps(1, heaps);

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
        audio->Play("select", /*loop=*/false, selectVol);
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        audio->Stop("select");
      }

      ImGui::End();
    }

    // ImGuiの内部コマンドを生成する
    ImGui::Render();

    input->Update();

    // 既存のデバッグ出力
    if (input->WasPressed(DIK_0) || input->WasMousePressed(0) ||
        input->WasPadPressed(XINPUT_GAMEPAD_A)) {
      OutputDebugStringA("Hit 0\n");
    }

    // カメラ更新も置き換え
    debugCamera->Update(*input);

    // カメラをキー入力ありの物に
    Matrix4x4 viewMatrix = debugCamera->GetViewMatrix();

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

    // ゲームの処理終わり

    // 描画先のRTVとDSVを設定する
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dx->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

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
    UINT backBufferIndex = dx->GetSwapChain()->GetCurrentBackBufferIndex();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        dx->GetRTVHandle(backBufferIndex); // ← 左辺値にする
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0,
                                   nullptr); // ← これも同じ変数を使ってOK
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
                                   0, nullptr);

    /*コマンドを積む
    -----------------------*/
    ID3D12GraphicsCommandList *cl = cmdList.Get();

    cmdList->RSSetViewports(1, &viewport);       // viewportを設定
    cmdList->RSSetScissorRects(1, &scissorRect); // Scissorを設定

    // モデル描画（内部で RS/PSO/CBV/SRV/VB/IB をまとめてセット）
    cmdList->SetGraphicsRootSignature(objPipeline.GetRootSignature());
    cmdList->SetPipelineState(objPipeline.GetPipelineState());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    model.Draw(viewMatrix, projectionMatrix, directionalLightResource.Get());
    planeModel.Draw(viewMatrix, projectionMatrix,
                    directionalLightResource.Get());

    /* 2D描画コマンドを積む
    -----------------------------*/
    // Sprite 描画（内部でPSO/RS/VB/IB/CBV/SRVすべて設定）
    cmdList->SetGraphicsRootSignature(
        spritePipeline.GetRootSignature()); // ✅ Sprite用RootSig
    cmdList->SetPipelineState(
        spritePipeline.GetPipelineState()); // ✅ Sprite用PSO
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Matrix4x4 view = MakeIdentity4x4();
    Matrix4x4 proj = MakeOrthographicMatrix(
        0.0f, 0.0f, static_cast<float>(app->kClientWidth),
        static_cast<float>(app->kClientHeight), 0.0f, 1.0f);

    sprite.Draw(view, proj);

    /* ImGuiを描画
    -------------------------------*/
    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

    engine.EndFrame();
  }

  /*解放処理
  --------------------*/
  // 音声データ開
  delete debugCamera;
  engine.Finalize();

  CoUninitialize();

  return 0;
}
