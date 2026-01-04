#pragma once
#include "Vector.h"

class GameCamera;

struct CameraContext {
    Vector3 playerPos{};
    Vector3 playerForward{};
    Vector3 enemyPos{};
    bool hasEnemy = false;
    float deltaTime = 0.0f;
};

class ICameraController {
public:
    virtual ~ICameraController() = default;
    virtual void OnEnter(GameCamera& camera, const CameraContext& ctx) {}
    virtual void Update(GameCamera& camera, const CameraContext& ctx) = 0;
    virtual void OnExit(GameCamera& camera, const CameraContext& ctx) {}
};
