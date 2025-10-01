#include "DebugCamera.h"

void DebugCamera::Initialize() {}

void DebugCamera::Update(const Input &input) {
  /* カメラの移動
  ----------------------------------*/
  const float speed = 0.05f;

  // カメラの移動ベクトル
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

  // 回転を適用して、カメラの向きに合わせて移動
  move = TransformNormal(move, matRot_);
  translate_ = Add(translate_, move);

  /* カメラの回転
  -------------------------------*/
  const float rotSpeed = 0.05f; // 回転速度（ラジアン）
  Matrix4x4 rotDelta = MakeIdentity4x4();

  // 左右キーでY軸回転
  if (input.IsDown(DIK_LEFT)) {
    rotDelta = Multiply(MakeRotateYMatrix(rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_RIGHT)) {
    rotDelta = Multiply(MakeRotateYMatrix(-rotSpeed), rotDelta);
  }

  // 上下キーでX軸回転
  if (input.IsDown(DIK_UP)) {
    rotDelta = Multiply(MakeRotateXMatrix(rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_DOWN)) {
    rotDelta = Multiply(MakeRotateXMatrix(-rotSpeed), rotDelta);
  }

  if (input.IsDown(DIK_Z)) {
    rotDelta = Multiply(MakeRotateZMatrix(rotSpeed), rotDelta);
  }
  if (input.IsDown(DIK_C)) {
    rotDelta = Multiply(MakeRotateZMatrix(-rotSpeed), rotDelta);
  }

  // 合成（順序に注意！Y→X→Z）
  matRot_ = Multiply(rotDelta, matRot_);

  Vector3 cameraPosition = TransformNormal(translate_, matRot_);

  // 平行移動行列
  Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPosition);

  // ワールド行列 = 回転 × 平行移動
  Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

  viewMatrix_ = Inverse(worldMatrix);
}
