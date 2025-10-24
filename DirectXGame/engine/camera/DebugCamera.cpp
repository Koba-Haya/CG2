#include "DebugCamera.h"

void DebugCamera::Initialize() {}

void DebugCamera::Update(const Input &input) {
  const float speed = 0.05f;
  Vector3 move = {0.0f, 0.0f, 0.0f};

  if (input.IsDown(DIK_W)) {
    move.z += speed;
  }
  if (input.IsDown(DIK_A)) {
    move.x -= speed;
  }
  if (input.IsDown(DIK_S)) {
    move.z -= speed;
  }
  if (input.IsDown(DIK_D)) {
    move.x += speed;
  }
  if (input.IsDown(DIK_Q)) {
    move.y += speed;
  }
  if (input.IsDown(DIK_E)) {
    move.y -= speed;
  }

  move = TransformNormal(move, matRot_);
  translate_ = Add(translate_, move);

  const float rotSpeed = 0.05f;
  Matrix4x4 rotDelta = MakeIdentity4x4();

  if (input.IsDown(DIK_LEFT)) {
    rotDelta = Multiply(MakeRotateYMatrix(+rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_RIGHT)) {
    rotDelta = Multiply(MakeRotateYMatrix(-rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_UP)) {
    rotDelta = Multiply(MakeRotateXMatrix(+rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_DOWN)) {
    rotDelta = Multiply(MakeRotateXMatrix(-rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_Z)) {
    rotDelta = Multiply(MakeRotateZMatrix(+rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_C)) {
    rotDelta = Multiply(MakeRotateZMatrix(-rotSpeed), rotDelta);
  }

  matRot_ = Multiply(rotDelta, matRot_);
  Vector3 cameraPosition = TransformNormal(translate_, matRot_);

  Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPosition);
  Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);
  viewMatrix_ = Inverse(worldMatrix);
}
