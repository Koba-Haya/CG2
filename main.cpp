#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem> //ファイルやディレクトリに関する操作を行うライブラリ
#include <fstream> //ファイルに書いたり読んだりするライブラリ
#include <chrono> //時間を扱うライブラリ

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

void Log(std::ostream& os, const std::string& message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
}

// 文字列を格納する
std::string str0{"STRING!!!"};

//整数を文字列にする
std::string str1{std::to_string(10)};

// 現在時刻を取得(UTC時刻)
std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

//日本時間(PCの設定時間)に変換
std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};

// formatを使って年月日_時分秒の文字列に変換
std::string dateString = std::format("{:%Y%m%d_%H%M&S}", localTime);

//時刻を使ってファイル名を決定
std::string logFilePath = std::string("logs/") + dateString + ".log";

//ファイルを作って書き込み準備
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

  int enemyHp = 0;   //ログに出る値
  int texturePath = 0;
  int wstringValue = 0;

  //変数から型を推論してくれる
  Log(std::format("enemyHp:{}, texturePath:{}\n", enemyHp, texturePath));

  //wstring->string
  Log(ConvertString(std::format(L"WSTRING{}\n",wstringValue)));

  // ログのディレクトリを用意
  std::filesystem::create_directory("logs");

  // ウィンドウの×ボタンが押されるまでループ
  while (msg.message != WM_QUIT) {
    // Windowにメッセージが来ていたら最優先で処理させる
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // ゲームの処理
    }
  }

  return 0;
}
