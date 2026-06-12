#include "Player/Player.h"
#include "Input.h"
#include "GameCamera.h"
#include "Method.h"
#include <algorithm>

void Player::Initialize(std::shared_ptr<ModelResource> modelRes) {
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  modelInstance_.Initialize({ modelRes, {1, 1, 1, 1}, 0 });
  localPos_ = { 0.0f, -1.0f }; // 初期位置：少し下側
  hp_ = maxHp_;
}

void Player::Update(const Input &input, const GameCamera &camera, float deltaTime) {
  // 入力によるローカル座標の移動
  float step = moveSpeed_ * deltaTime;
  if (input.PressKey(DIK_W) || input.PressKey(DIK_UP)) {
    localPos_.y += step;
  }
  if (input.PressKey(DIK_S) || input.PressKey(DIK_DOWN)) {
    localPos_.y -= step;
  }
  if (input.PressKey(DIK_A) || input.PressKey(DIK_LEFT)) {
    localPos_.x -= step;
  }
  if (input.PressKey(DIK_D) || input.PressKey(DIK_RIGHT)) {
    localPos_.x += step;
  }

  // 画面外に出ないようにクランプ（視野角0.45rad、距離10mでの可視領域はおよそ横±4.0、縦±2.28。モデルの大きさを考慮して横±3.5、縦±1.8とする）
  localPos_.x = std::clamp(localPos_.x, -3.5f, 3.5f);
  localPos_.y = std::clamp(localPos_.y, -1.8f, 1.8f);

  // カメラの情報を取得
  Vector3 eye = camera.GetEye();
  Vector3 forward = camera.GetForward();
  Vector3 right = camera.GetRight();
  Vector3 up = camera.GetActualUp();

  // ワールド座標の計算: Eye + Forward*Dist + Right*localX + Up*localY
  Vector3 forwardOffset = { forward.x * cameraDistance_, forward.y * cameraDistance_, forward.z * cameraDistance_ };
  Vector3 rightOffset = { right.x * localPos_.x, right.y * localPos_.x, right.z * localPos_.x };
  Vector3 upOffset = { up.x * localPos_.y, up.y * localPos_.y, up.z * localPos_.y };

  worldPos_ = {
      eye.x + forwardOffset.x + rightOffset.x + upOffset.x,
      eye.y + forwardOffset.y + rightOffset.y + upOffset.y,
      eye.z + forwardOffset.z + rightOffset.z + upOffset.z
  };

  // プレイヤーモデルのワールド行列を作成（カメラと同じ姿勢）
  Matrix4x4 mat;
  mat.m[0][0] = -right.x; mat.m[0][1] = -right.y; mat.m[0][2] = -right.z; mat.m[0][3] = 0.0f;
  mat.m[1][0] = up.x;    mat.m[1][1] = up.y;    mat.m[1][2] = up.z;    mat.m[1][3] = 0.0f;
  mat.m[2][0] = forward.x; mat.m[2][1] = forward.y; mat.m[2][2] = forward.z; mat.m[2][3] = 0.0f;
  mat.m[3][0] = worldPos_.x; mat.m[3][1] = worldPos_.y; mat.m[3][2] = worldPos_.z; mat.m[3][3] = 1.0f;

  modelInstance_.SetWorld(mat);
}

void Player::Draw() {
  modelInstance_.Draw();
}

void Player::TakeDamage(int damage) {
  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }
}

bool Player::IsDead() const {
  return hp_ <= 0;
}