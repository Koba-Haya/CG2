#pragma once

#include "Camera.h"
#include "Input.h"
#include "Method.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")

namespace AbsoluteEngine {

// 汎用の自由視点カメラ（右ドラッグ視点+WASDQE移動+中ドラッグパン+ホイールズーム）。
// シーン編集用のナビゲーションカメラとしても、ゲーム内のデバッグ/フリーカメラとしても
// 使えるよう、移動・回転の挙動を下記のセッターで調整できるようにしてある
// （以前は用途ごとにDebugCameraという別クラスがほぼ同じ実装を持っていたが、EditorCameraに統合した）
class EditorCamera final : public Camera {
public:
  void Initialize() override;
  void Update(const Input &input) override;

  // 注視点にフォーカスする
  void FocusOn(const Vector3& targetPosition, float distance = 10.0f);

  Vector3 GetTranslate() const { return translate_; }
  void SetTranslate(const Vector3& t) { translate_ = t; }
  Vector3 GetRotation() const { return {pitch_, yaw_, roll_}; }
  void SetRotation(const Vector3& r) {
      pitch_ = r.x; yaw_ = r.y; roll_ = r.z;
  }

  void SetShakeOffset(const Vector3& offset) { shakeOffset_ = offset; }
  void UpdateMatrix();

  // WASDQE移動・ホイールズームに右ドラッグ中であることを要求するか
  // （シーン編集中のナビゲーションでは誤操作防止のためtrueが既定。常時自由に動けるゲーム内
  //   デバッグカメラとして使う場合はfalseにする）
  void SetRequireMouseForMovement(bool require) { requireMouseForMovement_ = require; }

  // 矢印キー/Z/Cキーによる回転（Yaw/Pitch/Roll）を有効にするか（既定は無効、マウスドラッグのみ）
  void SetKeyboardRotationEnabled(bool enabled) { keyboardRotationEnabled_ = enabled; }

  // 移動速度（キー移動・中ドラッグパン・ホイールズーム）を調整する
  void SetMovementSpeeds(float keyMoveSpeed, float mousePanSpeed, float wheelMoveSpeed) {
      keyMoveSpeed_ = keyMoveSpeed;
      mousePanSpeed_ = mousePanSpeed;
      wheelMoveSpeed_ = wheelMoveSpeed;
  }

private:
  Matrix4x4 matRot_ = MakeIdentity4x4();
  Vector3 translate_ = {0.0f, 0.0f, -20.0f};

  float yaw_ = 0.0f;   // 左右回転（Y）
  float pitch_ = 0.0f; // 上下回転（X）
  float roll_ = 0.0f;  // Z回転（必要なら）

  Vector3 shakeOffset_ = {0.0f, 0.0f, 0.0f};

  bool requireMouseForMovement_ = true;
  bool keyboardRotationEnabled_ = false;
  float keyMoveSpeed_ = 0.05f;
  float mousePanSpeed_ = 0.02f;
  float wheelMoveSpeed_ = 0.002f;
};

} // namespace AbsoluteEngine
