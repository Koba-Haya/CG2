#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <wrl.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  Input();
  ~Input();

  // 初期化と終了
  bool Initialize(HINSTANCE instanceHandle, HWND windowHandle);
  void Finalize();

  // 毎フレーム更新（Acquire と GetDeviceState を内部で実行）
  void Update();

  // 状態参照（DirectInput の DIK_* をそのまま渡せる）
  bool IsDown(int dik) const;      // 押されている間 true
  bool WasPressed(int dik) const;  // 今フレームで押された瞬間 true
  bool WasReleased(int dik) const; // 今フレームで離された瞬間 true

  // 互換用：生の配列を取得（既存コードの移行途中で使える）
  const BYTE *GetKeyboardState() const;

private:
  // コピー禁止
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;

private:
  ComPtr<IDirectInput8> directInput_ = nullptr;
  ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

  BYTE currentKeys_[256] = {};
  BYTE previousKeys_[256] = {};
  bool isInitialized_ = false;
};
