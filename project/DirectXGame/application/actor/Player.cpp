#include "Player.h"
#include "Input.h"
#include "Method.h"
#include "Renderer.h"

void Player::Initialize(std::shared_ptr<ModelResource> resource) {
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  
  ModelInstance::CreateInfo ci{};
  ci.resource = resource;
  modelInstance_.Initialize(ci);
}

void Player::Update(const Input &input) {
  if (input.PressKey(DIK_W)) {
    transform_.translate.z += moveSpeed_;
  }
  if (input.PressKey(DIK_S)) {
    transform_.translate.z -= moveSpeed_;
  }
  if (input.PressKey(DIK_A)) {
    transform_.translate.x -= moveSpeed_;
  }
  if (input.PressKey(DIK_D)) {
    transform_.translate.x += moveSpeed_;
  }

  Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
  modelInstance_.SetWorld(world);
}

void Player::Draw() const {
  Renderer::GetInstance()->DrawModel(const_cast<ModelInstance*>(&modelInstance_));
}