#pragma once
#include "Transform.h"
#include "ModelInstance.h"
#include <memory>

class Input;

class Player {
public:
  void Initialize(std::shared_ptr<ModelResource> resource);
  void Update(const Input &input);
  void Draw() const;

  void SetPosition(const Vector3 &pos) { transform_.translate = pos; }
  void SetRotation(const Vector3 &rot) { transform_.rotate = rot; }
  const Vector3& GetPosition() const { return transform_.translate; }
  const Vector3& GetRotation() const { return transform_.rotate; }
  const Transform &GetTransform() const { return transform_; }

private:
  Transform transform_;
  ModelInstance modelInstance_;
  float moveSpeed_ = 0.1f;
};
