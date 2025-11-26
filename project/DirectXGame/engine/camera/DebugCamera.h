#pragma once
#include "Input.h" // ← 追加
#include "Method.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")

class DebugCamera {
  Matrix4x4 matRot_ = MakeIdentity4x4();
  Vector3 translate_ = {0.0f, 0.0f, -20.0f};
  Matrix4x4 viewMatrix_ = MakeIdentity4x4();
  Matrix4x4 projectionMatrix_ = MakeIdentity4x4();

public:
  void Initialize();

  void Update(const Input &input); // ← ここを差し替え

  Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
  Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }
};
