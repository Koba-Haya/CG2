#include "Player.h"
#include "Input.h"

void Player::Initialize() {
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
}

void Player::Update(const Input &input) {
  if (input.IsDown(DIK_W)) {
    transform_.translate.z += moveSpeed_;
  }
  if (input.IsDown(DIK_S)) {
    transform_.translate.z -= moveSpeed_;
  }
  if (input.IsDown(DIK_A)) {
    transform_.translate.x -= moveSpeed_;
  }
  if (input.IsDown(DIK_D)) {
    transform_.translate.x += moveSpeed_;
  }
}

void Player::Draw() const {}