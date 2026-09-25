#pragma once
#include "Vector.h"
#include <vector>

class GameCamera;

// カメラが注視・追従しうる1つの対象（位置と向き）
// 「誰を画面に収めるか・何体収めるか」はエンジンでは決めず、シーン側がCameraContext::focusTargets
// に詰める内容で決める（1人称/1体追従なら1件、複数体を画面に収めたいなら複数件、
// 対象なしの固定カメラなら0件、という形で表現する）
struct CameraFocusTarget {
    Vector3 position{};
    Vector3 forward{};
};

struct CameraContext {
    std::vector<CameraFocusTarget> focusTargets;
    float deltaTime = 0.0f;
};

class ICameraController {
public:
    virtual ~ICameraController() = default;
    virtual void OnEnter(GameCamera& camera, const CameraContext& ctx) {}
    virtual void Update(GameCamera& camera, const CameraContext& ctx) = 0;
    virtual void OnExit(GameCamera& camera, const CameraContext& ctx) {}
};
