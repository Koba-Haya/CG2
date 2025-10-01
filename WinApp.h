#pragma once
#include <Windows.h> // LRESULT, HWND, HINSTANCE など Windows API の型
#include <string>

class WinApp {
public:
  // 静的メンバ関数
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam);

public:
  void Initalize();
  void Update();
  void Finalize();
  bool ProcessMessage();

  // クライアント領域のサイズ
  static const int32_t kClientWidth = 1280;
  static const int32_t kClientHeight = 720;

  // ゲッター
  HWND GetHwnd() const { return hwnd; };
  HINSTANCE GetHInstance() const { return wc.hInstance; };

private:
  // ウィンドウハンドル
  HWND hwnd = nullptr;

  // ウィンドウクラスの設定
  WNDCLASS wc{};
};

//// アプリのウィンドウ生成・メッセージ処理をまとめたクラス
// class WinApp {
// public:
//   // コンストラクタで希望のクライアントサイズとタイトルを受け取る
//   WinApp(int width, int height, std::wstring className = L"CG2WindowClass",
//          std::wstring title = L"CG2");
//   ~WinApp();
//
//   // ウィンドウを作成して表示する（RegisterClass → CreateWindow →
//   ShowWindow） bool Create();
//
//   // 毎フレーム呼ぶ。Windowsメッセージを処理して、
//   // WM_QUIT が出たら false を返す（＝メインループ終了）
//   bool ProcessMessages(MSG *outMsg = nullptr);
//
//   // ウィンドウ破棄とクラスの登録解除
//   void Destroy();
//
//   // 他のクラスが必要とする基本情報を公開
//   HINSTANCE GetHInstance() const { return hInstance_; }
//   HWND GetHwnd() const { return hwnd_; }
//   int Width() const { return width_; }
//   int Height() const { return height_; }
//
// private:
//     // CreateWindow が最初に呼ぶ静的な WndProc
//   // ここで this を hwnd に結び付けて、以降の処理をインスタンスメソッドに回す
//   static LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam,
//                                        LPARAM lParam);
//
//   // 実際の WndProc 処理（インスタンスメソッド）
//   LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM
//   lParam);
//
// private:
//   HINSTANCE hInstance_ = nullptr; // アプリのインスタンスハンドル
//   HWND hwnd_ = nullptr;           // 作成されたウィンドウハンドル
//   std::wstring className_;        // WNDCLASS 登録時の名前
//   std::wstring title_;            // ウィンドウタイトル
//   int width_ = 1280;              // 希望クライアント幅
//   int height_ = 720;              // 希望クライアント高さ
//   bool created_ = false;          // ウィンドウが作成済みかどうか
// };
