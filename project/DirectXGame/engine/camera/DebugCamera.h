#pragma once

#include "Camera.h"
#include "Input.h"
#include "Method.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")

class DebugCamera final : public Camera {
public:
  void Initialize() override;
  void Update(const Input &input) override;

private:
  Matrix4x4 matRot_ = MakeIdentity4x4();
  Vector3 translate_ = {0.0f, 0.0f, -20.0f};

  float yaw_ = 0.0f;   // 左右回転（Y）
  float pitch_ = 0.0f; // 上下回転（X）
  float roll_ = 0.0f;  // Z回転（必要なら）
};
