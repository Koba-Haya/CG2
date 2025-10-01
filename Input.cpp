#include "Input.h"
#define DIRECTINPUT_VERSION 0x0800
#include <cassert>
#include <dinput.h>
#include <wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma commnet(lib, "dxguid.lib")

using namespace Microsoft::WRL;

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
  ComPtr<IDirectInputDevice8> keyboard = nullptr;
  hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  assert(SUCCEEDED(hr));

  // 入力データ形式のセット
  hr = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  hr = keyboard->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));
}

void Input::Update() {}
