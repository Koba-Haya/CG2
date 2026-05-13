#pragma once

#include "Vector.h"

/// シーン層が使うライト記述子（D3D12 非依存）

struct DirLight {
    Vector3 color = {1.0f, 1.0f, 1.0f};
    Vector3 direction = {0.0f, -1.0f, 0.0f};
    float intensity = 1.0f;
    bool enabled = true;
};

struct PointLight {
    Vector3 color = {1.0f, 1.0f, 1.0f};
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float intensity = 1.0f;
    float radius = 10.0f;
    float decay = 2.0f;
    bool enabled = true;
};

struct SpotLight {
    Vector3 color = {1.0f, 1.0f, 1.0f};
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 direction = {0.0f, -1.0f, 0.0f};
    float intensity = 1.0f;
    float distance = 10.0f;
    float decay = 2.0f;
    float coneAngleDeg = 30.0f;
    bool enabled = true;
};
