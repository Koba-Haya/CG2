#pragma once
#include "Vector.h"

struct EulerTransform {
  Vector3 scale{1.0f, 1.0f, 1.0f};
  Vector3 rotate{0.0f, 0.0f, 0.0f};
  Vector3 translate{0.0f, 0.0f, 0.0f};
};

using Transform = EulerTransform;

struct QuaternionTransform {
  Vector3 scale{1.0f, 1.0f, 1.0f};
  Quaternion rotate{0.0f, 0.0f, 0.0f, 1.0f};
  Vector3 translate{0.0f, 0.0f, 0.0f};
};
