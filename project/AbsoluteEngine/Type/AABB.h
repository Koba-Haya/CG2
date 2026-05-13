#pragma once
#include "Vector.h"

struct AABB {
  Vector3 min; // 最小点
  Vector3 max; // 最大点
};

struct OBB {
  Vector3 center;          // 中心点
  Vector3 orientations[3]; // 座標軸。正規化・直行必要
  Vector3 size;            // 中心点から面までの距離
};
