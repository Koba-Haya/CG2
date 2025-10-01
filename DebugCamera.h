#pragma once
#include "Method.h"
#include "Input.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")

class DebugCamera {
  // 累積回転行列
  Matrix4x4 matRot_ = MakeIdentity4x4();
  // ローカル座標
  Vector3 translate_ = {0.0f, 0.0f, -30.0f};
  // ビュー行列
  Matrix4x4 viewMatrix_ = MakeIdentity4x4();
  // 射影行列
  Matrix4x4 projectionMatrix_ = MakeIdentity4x4();

public:
  void Initialize();

  void Update(const Input &input);

  Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
};
