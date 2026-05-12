#pragma once
#include "Vector.h"
#include <cstdint>

struct GPUParticle {
    Vector3 translate;
    Vector3 scale;
    float lifeTime;
    Vector3 velocity;
    float currentTime;
    Vector4 color;
};
