#include "WinApp.h"

// ImGui の Win32 バックエンドを使うためのヘッダ
// → h ファイルには書かず cpp にだけ書く（依存を最小化するため）
#include "externals/imgui/imgui.h"
// #include "externals/imgui/imgui_impl_win32.h"

// extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
//                                                              UINT msg,
//                                                              WPARAM wParam,
//                                                              LPARAM lParam);
//
// WinApp::WinApp(int width, int height, std::wstring className,
//                std::wstring title)
//     : className_(std::move(className)), title_(std::move(title)),
//     width_(width),
//       height_(height) {
//   // アプリの HINSTANCE を取得して保持
//   hInstance_ = GetModuleHandle(nullptr);
// }
//
// WinApp::~WinApp() { Destroy(); }
//
// bool WinApp::Create() {
//   // 1) WNDCLASS 構造体を埋めて RegisterClass
//   WNDCLASS wc{};
//   wc.lpfnWndProc = &WinApp::WndProcSetup; // 最初は setup 用 WndProc を使う
//   wc.lpszClassName = className_.c_str();       // クラス名
//   wc.hInstance = hInstance_;                   // アプリインスタンス
//   wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // デフォルトカーソル
//   if (!RegisterClass(&wc)) {
//     return false;
//   }
//
//   // 2) クライアント領域サイズを指定してウィンドウ矩形を計算
//   RECT rc{0, 0, width_, height_};
//   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
//
//   // 3) CreateWindow でウィンドウ生成
//   // lParam に this を渡して、WndProcSetup 側で拾えるようにする
//   hwnd_ = CreateWindow(className_.c_str(), title_.c_str(),
//   WS_OVERLAPPEDWINDOW,
//                        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
//                        rc.bottom - rc.top, nullptr, nullptr, hInstance_,
//                        this);
//   if (!hwnd_) {
//     return false;
//   }
//
//   // 4) ウィンドウ表示
//   ShowWindow(hwnd_, SW_SHOW);
//   created_ = true;
//   return true;
// }
//
// bool WinApp::ProcessMessages(MSG *outMsg) {
//   MSG msg{};
//   while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
//     // ImGui が処理したメッセージはスキップ
//     if (ImGui_ImplWin32_WndProcHandler(msg.hwnd, msg.message, msg.wParam,
//                                        msg.lParam)) {
//       continue;
//     }
//     TranslateMessage(&msg);
//     DispatchMessage(&msg);
//
//     // WM_QUIT が来たら false を返してループ終了
//     if (msg.message == WM_QUIT) {
//       if (outMsg) {
//         *outMsg = msg;
//       }
//       return false;
//     }
//   }
//   if (outMsg) {
//     *outMsg = msg;
//   }
//   return true;
// }
//
// void WinApp::Destroy() {
//   if (hwnd_) {
//     DestroyWindow(hwnd_); // ウィンドウ破棄
//     hwnd_ = nullptr;
//   }
//   if (!className_.empty() && hInstance_) {
//     UnregisterClass(className_.c_str(), hInstance_); // クラス登録解除
//   }
//   created_ = false;
// }
//
//// 最初に呼ばれる WndProc
//// ここで this を hwnd の GWLP_USERDATA
//// に関連付け、以降はインスタンスメソッドに転送
// LRESULT CALLBACK WinApp::WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam,
//                                       LPARAM lParam) {
//   if (msg == WM_NCCREATE) {
//     // CREATESTRUCT に this が渡されている
//     CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
//     WinApp *that = static_cast<WinApp *>(cs->lpCreateParams);
//     that->hwnd_ = hwnd;
//     // hwnd に this を保存
//     SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
//   }
//
//   // 保存済みの this を取り出してメンバ関数に転送
//   if (WinApp *that =
//           reinterpret_cast<WinApp *>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
//           {
//     return that->WndProc(hwnd, msg, wParam, lParam);
//   }
//
//   // まだ this が設定されていない段階ではデフォルト処理
//   return DefWindowProc(hwnd, msg, wParam, lParam);
// }
//
//// 実際の WndProc（インスタンスメソッド）
// LRESULT CALLBACK WinApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam,
//                                  LPARAM lParam) {
//   switch (msg) {
//   case WM_DESTROY:
//     PostQuitMessage(0); // アプリ終了
//     return 0;
//   default:
//     break;
//   }
//   return DefWindowProc(hwnd, msg, wParam, lParam);
// }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                    LPARAM lparam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }
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

void WinApp::Initalize() {

  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  assert(SUCCEEDED(hr));

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

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域を元に実際のサイズに、wrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  hwnd = CreateWindow(wc.lpszClassName, // 利用するクラス名
                      L"CG2",           // タイトルバーの文字
                      WS_OVERLAPPEDWINDOW, // よく見るウィンドウスタイル
                      CW_USEDEFAULT,       // 表示X座標(Windowsに任せる)
                      CW_USEDEFAULT,       // 表示X座標(Windowsに任せる)
                      wrc.right - wrc.left, // ウィンドウ横幅
                      wrc.bottom - wrc.top, // ウィンドウ縦幅
                      nullptr,              // 親ウィンドウハンドル
                      nullptr,              // メニューハンドル
                      wc.hInstance,         // インスタンスハンドル
                      nullptr);             // オプション

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);
}

void WinApp::Update() {}

void WinApp::Finalize() {

  if (hwnd) {
    DestroyWindow(hwnd);
    hwnd = nullptr;
  }
  if (wc.lpszClassName && wc.hInstance) {
    UnregisterClass(wc.lpszClassName, wc.hInstance);
  }
  CoUninitialize();
}

bool WinApp::ProcessMessage() {
  MSG msg{};

  // Windowにメッセージが来ていたら最優先で処理させる
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    return true;
  }

  TranslateMessage(&msg);
  DispatchMessage(&msg);

  return false;
}