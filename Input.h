#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <cassert>
#include <dinput.h>
#include <wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <Windows.h>

class Input {
public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  void Initialize(HINSTANCE hInstance, HWND hwnd);
  void Update();

private:
  ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
};
