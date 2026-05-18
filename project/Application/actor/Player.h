#pragma once
#include "Transform.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include <memory>

class Input;
class GameCamera;

class Player {
public:
  void Initialize(std::shared_ptr<ModelResource> modelRes);
  void Update(const Input &input, const GameCamera &camera, float deltaTime);
  void Draw();

  const Transform &GetTransform() const { return transform_; }
  const Vector3 &GetWorldPosition() const { return worldPos_; }

private:
  Transform transform_;
  ModelInstance modelInstance_;
  Vector3 worldPos_{ 0.0f, 0.0f, 0.0f };

  // カメラ空間でのローカル座標
  Vector2 localPos_{ 0.0f, 0.0f };
  float moveSpeed_ = 10.0f;
  float cameraDistance_ = 10.0f; // カメラ前方どれくらいに配置するか
};
