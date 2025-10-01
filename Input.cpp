#include "Input.h"
#include <cassert>
#include <cstring>

Input::Input() {}
Input::~Input() { Finalize(); }

bool Input::Initialize(HINSTANCE instanceHandle, HWND windowHandle) {

  // DirectInputの初期化
  HRESULT hr = DirectInput8Create(
      instanceHandle, DIRECTINPUT_VERSION, IID_IDirectInput8,
      reinterpret_cast<void **>(directInput_.ReleaseAndGetAddressOf()),
      nullptr);

  assert(SUCCEEDED(hr));

  // キーボードデバイスの生成
  hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
  assert(SUCCEEDED(hr));

  // 入力データ形式のセット
  hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  // フォアグラウンド・非排他・Windowsキー無効（従来と同じ）
  hr = keyboard_->SetCooperativeLevel(
      windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));

  // 前フレームを保存
  std::memset(currentKeys_, 0, sizeof(currentKeys_));
  std::memset(previousKeys_, 0, sizeof(previousKeys_));
  isInitialized_ = true;
  return true;
}

void Input::Finalize() {
  if (keyboard_) {
    keyboard_->Unacquire();
    keyboard_->Release();
    keyboard_ = nullptr;
  }
  if (directInput_) {
    directInput_->Release();
    directInput_ = nullptr;
  }
  isInitialized_ = false;
}

void Input::Update() {
  if (!isInitialized_) {
    return;
  }

  // 前フレームを保存
  std::memcpy(previousKeys_, currentKeys_, sizeof(currentKeys_));

  // 取得開始
  keyboard_->Acquire();
  std::memset(currentKeys_, 0, sizeof(currentKeys_));
  keyboard_->GetDeviceState(sizeof(currentKeys_), currentKeys_);
}

// 押している間
bool Input::IsDown(int dik) const { return (currentKeys_[dik] & 0x80) != 0; }

// 押した瞬間
bool Input::WasPressed(int dik) const {
  bool now = (currentKeys_[dik] & 0x80) != 0;
  bool prev = (previousKeys_[dik] & 0x80) != 0;
  return now && !prev;
}

// 離した瞬間
bool Input::WasReleased(int dik) const {
  bool now = (currentKeys_[dik] & 0x80) != 0;
  bool prev = (previousKeys_[dik] & 0x80) != 0;
  return !now && prev;
}

const BYTE *Input::GetKeyboardState() const { return currentKeys_; }