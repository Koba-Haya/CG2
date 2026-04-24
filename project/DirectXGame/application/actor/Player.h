#pragma once
#include "Transform.h"

class Input;

class Player {
public:
  void Initialize();
  void Update(const Input &input);
  void Draw() const;

  const Transform &GetTransform() const { return transform_; }

private:
  Transform transform_;
  float moveSpeed_ = 5.0f;
};
