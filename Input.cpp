#include "Input.h"

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
  /* DirectInputオブジェクトの生成
  ----------------------------------*/
  HRESULT hr;

  // DirectInputの初期化
  ComPtr<IDirectInput8> directInput = nullptr;
  hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
                          (void **)&directInput, nullptr);
  assert(SUCCEEDED(hr));

  // キーボードデバイスの生成
  hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
  assert(SUCCEEDED(hr));

  // 入力データ形式のセット
  hr = keyboard_->SetDataFormat(&c_dfDIKeyboard); // 標準形式
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  hr = keyboard_->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));
}

void Input::Update() {
  /* キーボード情報の取得
  -----------------------------------*/
  // キーボード情報の取得開始
  keyboard_->Acquire();

  // 全キーの入力状態を取得する
  BYTE key[256] = {};
  keyboard_->GetDeviceState(sizeof(key), key);
}
