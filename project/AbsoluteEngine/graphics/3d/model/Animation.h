#pragma once

#include <vector>
#include <map>
#include <string>
#include "Vector.h"
#include "Method.h"

template <typename tValue>
struct Keyframe {
  float time;
  tValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template <typename tValue>
struct AnimationCurve {
  std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
  AnimationCurve<Vector3> translate;
  AnimationCurve<Quaternion> rotate;
  AnimationCurve<Vector3> scale;
};

struct Animation {
  float duration; // アニメーション全体の尺（単位は秒）
  // NodeAnimationの集合。Node名で引けるようにしておく
  std::map<std::string, NodeAnimation> nodeAnimations;
};
