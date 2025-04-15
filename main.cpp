#include <Windows.h>
#include <cassert>
#include <chrono> //時間を扱うライブラリ
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <filesystem> //ファイルやディレクトリに関する操作を行うライブラリ
#include <format>
#include <fstream> //ファイルに書いたり読んだりするライブラリ
#include <string>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  // メッセージに応じてゲーム固有の処理を行う
  switch (msg) {
    // ウィンドウが破棄された
  case WM_DESTROY:
    // DSに対して、アプリの終了を伝える
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

void Log(std::ostream &os, const std::string &message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
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
std::string dateString = std::format("{:%Y%m%d_%H%M&S}", localTime);

// 時刻を使ってファイル名を決定
std::string logFilePath = std::string("logs/") + dateString + ".log";

// ファイルを作って書き込み準備
std::ofstream lobStream(logFilePath);

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  WNDCLASS wc{};

  // ウィンドウプロシージャ
  wc.lpfnWndProc = WindowProc;

  // ウィンドウクラス名
  wc.lpszClassName = L"CG2WindowClass";

  // インスタンスハンドル
  wc.hInstance = GetModuleHandle(nullptr);

  // カーソル
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc);

  // 出力ウィンドウへの文字出力
  OutputDebugStringA("Hello,DirectX!\n");

  // クライアント領域のサイズ
  const int32_t kClientWidth = 1280;
  const int32_t kClientHeight = 720;

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域を元に実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  HWND hwnd = CreateWindow(wc.lpszClassName, // 利用するクラス名
                           L"CG2",           // タイトルバーの文字
                           WS_OVERLAPPEDWINDOW, // よく見るウィンドウスタイル
                           CW_USEDEFAULT, // 表示X座標(Windowsに任せる)
                           CW_USEDEFAULT, // 表示X座標(Windowsに任せる)
                           wrc.right - wrc.left, // ウィンドウ横幅
                           wrc.bottom - wrc.top, // ウィンドウ縦幅
                           nullptr,      // 親ウィンドウハンドル
                           nullptr,      // メニューハンドル
                           wc.hInstance, // インスタンスハンドル
                           nullptr);     // オプション

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);

  MSG msg{};

  int enemyHp = 0; // ログに出る値
  int texturePath = 0;
  int wstringValue = 0;

  // 変数から型を推論してくれる
  Log(std::format("enemyHp:{}, texturePath:{}\n", enemyHp, texturePath));

  // wstring->string
  Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));

  // ログのディレクトリを用意
  std::filesystem::create_directory("logs");

  /* DXGIファクトリーの生成
  ---------------------------*/
  // DXGIファクトリーの生成
  IDXGIFactory7 *dxgiFactory = nullptr;

  // HRESULTはWindows系のエラーコード
  // 関数が成功したかどうかをSUCCEEDEDマクロで判定
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

  // 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
  assert(SUCCEEDED(hr));

  /* 使用するアダプタ(GPU)を決定する
  -----------------------------------*/
  // 使用するアダプタ用の変数。最初にnullptrを入れておく
  IDXGIAdapter4 *useAdapter = nullptr;

  // いい順にアダプタを頼む
  for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       ++i) {
    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr)); // 取得できないのは一大事

    // ソフトウェアアダプタでなければ採用！
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      // 採用したアダプタの情報をログに出力。wstringのほう
      Log(ConvertString(
          std::format(L"Use Adapter:{}\n",
                      adapterDesc.Description))); // ConvertString関数を追加した
      break;
    }
    useAdapter = nullptr; // ソフトウェアアダプタの場合はみなかったことにする
  }

  // 適切なアダプタが見つからなかったので起動できない
  assert(useAdapter != nullptr);

  /* D3D12Deviceの生成
  -------------------------------*/
  ID3D12Device *device = nullptr;

  // 機能レベルとログ出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};

  const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};

  // 高い順に生成できるか試していく
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターでデバイスを生成
    hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));

    // 指定した機能レベルでデバイスが生成できたか確認
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
      break;
    }
  }

  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);
  Log("Complete create D3D12Device!!!\n"); // 初期化完了のログを出す

  /* コマンドキューを生成する
  -------------------------------*/
  ID3D12CommandQueue *commandQueue = nullptr;
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));
  // コマンドキューの生成が上手くいかなかったので起動できない
  assert(SUCCEEDED(hr));

  /* コマンドリストを生成する
  -------------------------------*/
  // コマンドアロケータを生成する
  ID3D12CommandAllocator *commandAllocator = nullptr;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&commandAllocator));
  // コマンドアロケータの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドリストを生成する
  ID3D12GraphicsCommandList *commandList = nullptr;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator, nullptr,
                                 IID_PPV_ARGS(&commandList));
  // コマンドリストの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  /* スワップチェーンを生成する
  ------------------------------*/
  IDXGISwapChain4 *swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.Width =
      kClientWidth; // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
  swapChainDesc.Height =
      kClientHeight; // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
  swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
  swapChainDesc.BufferUsage =
      DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
  swapChainDesc.BufferCount = 2; // ダブルバッファ
  swapChainDesc.SwapEffect =
      DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニターに移したら、中身を破棄
  // コマンドキュー、ウィンドウハンドル、設定を渡して生成する
  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue, hwnd, &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(&swapChain));
  assert(SUCCEEDED(hr));

  /* ディスクリプタヒープの生成
  ------------------------------*/
  ID3D12DescriptorHeap *rtvDescriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
  rtvDescriptorHeapDesc.Type =
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー用
  rtvDescriptorHeapDesc.NumDescriptors =
      2; // ダブルバッファ用に2つ。多くても別にかまわない
  hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc,
                                    IID_PPV_ARGS(&rtvDescriptorHeap));
  // ディスクリプタヒープが作れなかったので起動できない
  assert(SUCCEEDED(hr));

  /*SwapChainからResourceを引っ張ってくる
  -----------------------------------------*/
  ID3D12Resource *swapChainResources[2] = {nullptr};
  hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
  // 上手く取得できなければ起動できない
  assert(SUCCEEDED(hr));
  hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
  assert(SUCCEEDED(hr));

  /* RTVを作る
  -------------------*/
  // RTVの設定
  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.ViewDimension =
      D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む
  // ディスクリプタの先頭を取得する
  D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
      rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  // RTVを2つ作るのでディスクリプタを2つ用意
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
  // まず1つを作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
  rtvHandles[0] = rtvStartHandle;
  device->CreateRenderTargetView(swapChainResources[0], &rtvDesc,
                                 rtvHandles[0]);
  // 2つ目のディスクリプタハンドルを得る(自力で)
  rtvHandles[1].ptr =
      rtvHandles[0].ptr +
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  // 2つ目を作る
  device->CreateRenderTargetView(swapChainResources[1], &rtvDesc,
                                 rtvHandles[1]);

  /* Descriptorの位置を決める
  ---------------------------------*/
  rtvHandles[0] = rtvStartHandle;
  // rtvHandles[1].ptr = rtvHandles[0].ptr +
  // device->GetDescriptorHandleIncrementSize();

  // ウィンドウの×ボタンが押されるまでループ
  while (msg.message != WM_QUIT) {
    // Windowにメッセージが来ていたら最優先で処理させる
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // ゲームの処理

      /*コマンドを積み込んで確定させる
      ------------------------------*/
      // これから書き込むバックバッファのインデクスを取得
      UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
      // 描画先のRTVを設定する
      commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                      nullptr);
      // 指定した色で画面全体をクリアする
      float clearColor[] = {
          0.1f,
          0.25f,
          0.5f,
          1.0f,
      }; // 青っぽい色。 RGBAの順
      commandList->ClearRenderTargetView(rtvHandles[backBufferIndex],
                                         clearColor, 0, nullptr);
      // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
      hr = commandList->Close();
      assert(SUCCEEDED(hr));

      /*コマンドをキックする
      ----------------------------*/
      // GPUにコマンドリストの実行を行わせる
      ID3D12CommandList *commandLists[] = {commandList};
      commandQueue->ExecuteCommandLists(1, commandLists);
      //GPUとOSに画面の交換を行うように通知する
      swapChain->Present(1, 0);
      //次のフレーム用のコマンドリストを準備
      hr = commandAllocator->Reset();
      assert(SUCCEEDED(hr));
      hr = commandList->Reset(commandAllocator, nullptr);
      assert(SUCCEEDED(hr));
    }
  }
  return 0;
}
