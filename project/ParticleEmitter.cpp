#define NOMINMAX

#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Method.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

namespace {
    std::mt19937& GetRNG() {
        static std::mt19937 rng{ std::random_device{}() };
        return rng;
    }

    float Clamp01(float v) {
        return std::clamp(v, 0.0f, 1.0f);
    }

    Vector3 HSVtoRGB(const Vector3& hsv) {
        float h = hsv.x;
        float s = hsv.y;
        float v = hsv.z;

        if (s <= 0.0f) {
            return { v, v, v };
        }

        h = std::fmod(h, 1.0f);
        if (h < 0.0f) {
            h += 1.0f;
        }

        float hf = h * 6.0f;
        int i = static_cast<int>(std::floor(hf));
        float f = hf - static_cast<float>(i);

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i % 6) {
        case 0: return { v, t, p };
        case 1: return { q, v, p };
        case 2: return { p, v, t };
        case 3: return { p, q, v };
        case 4: return { t, p, v };
        default: return { v, p, q };
        }
    }
} // namespace

ParticleEmitter::ParticleEmitter(ParticleManager* manager, const Params& params) {
    Initialize(manager, params);
}

void ParticleEmitter::Initialize(ParticleManager* manager, const Params& params) {
    manager_ = manager;
    params_ = params;
    emitAccum_ = 0.0f;
    assert(manager_);
}

void ParticleEmitter::Update(float deltaTime, const Vector3& parentTranslate) {
    if (!manager_ || params_.emitRate <= 0.0f) {
        return;
    }

    emitAccum_ += deltaTime * params_.emitRate;
    const uint32_t spawnCount = static_cast<uint32_t>(emitAccum_);
    if (spawnCount == 0) {
        return;
    }
    emitAccum_ -= static_cast<float>(spawnCount);

    for (uint32_t i = 0; i < spawnCount; ++i) {
        const Vector3 pos = SamplePosition_(parentTranslate);
        const Vector3 vel = SampleVelocity_();
        const float life = RandRange_(params_.lifeMin, params_.lifeMax);
        const Vector4 col = SampleColor_();

        manager_->Emit(params_.groupName, pos, vel, params_.particleScale, life, col);
    }
}

void ParticleEmitter::Burst(uint32_t count, const Vector3& parentTranslate) {
    if (!manager_ || count == 0) {
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const Vector3 pos = SamplePosition_(parentTranslate);
        const Vector3 vel = SampleVelocity_();
        const float life = RandRange_(params_.lifeMin, params_.lifeMax);
        const Vector4 col = SampleColor_();

        manager_->Emit(params_.groupName, pos, vel, params_.particleScale, life, col);
    }
}

Vector3 ParticleEmitter::SamplePosition_(const Vector3& parentTranslate) const {
    Vector3 pos = {
        parentTranslate.x + params_.localCenter.x,
        parentTranslate.y + params_.localCenter.y,
        parentTranslate.z + params_.localCenter.z
    };

    auto rndSigned = [this]() { return Rand01_() * 2.0f - 1.0f; };

    if (params_.shape == EmitterShape::Box) {
        Vector3 offset{
            rndSigned() * params_.extent.x,
            rndSigned() * params_.extent.y,
            rndSigned() * params_.extent.z,
        };
        pos.x += offset.x;
        pos.y += offset.y;
        pos.z += offset.z;
        return pos;
    }

    // Sphere内部（単位球内の棄却法）→ extentでスケール
    Vector3 radius = params_.extent;
    const float kMinRadius = 0.001f;
    radius.x = std::max(radius.x, kMinRadius);
    radius.y = std::max(radius.y, kMinRadius);
    radius.z = std::max(radius.z, kMinRadius);

    Vector3 local{};
    while (true) {
        local.x = RandRange_(-1.0f, 1.0f);
        local.y = RandRange_(-1.0f, 1.0f);
        local.z = RandRange_(-1.0f, 1.0f);
        const float len2 = local.x * local.x + local.y * local.y + local.z * local.z;
        if (len2 <= 1.0f) {
            break;
        }
    }

    pos.x += local.x * radius.x;
    pos.y += local.y * radius.y;
    pos.z += local.z * radius.z;

    return pos;
}

Vector3 ParticleEmitter::SampleVelocity_() const {
    auto rndSigned = [this]() { return Rand01_() * 2.0f - 1.0f; };

    Vector3 dir = params_.baseDir;
    dir.x += rndSigned() * params_.dirRandomness;
    dir.y += rndSigned() * params_.dirRandomness;
    dir.z += rndSigned() * params_.dirRandomness;

    if (Length(dir) > 0.0001f) {
        dir = Normalize(dir);
    } else {
        dir = { 0.0f, 1.0f, 0.0f };
    }

    float spd = RandRange_(params_.speedMin, params_.speedMax);
    return { dir.x * spd, dir.y * spd, dir.z * spd };
}

Vector4 ParticleEmitter::SampleColor_() const {
    switch (params_.colorMode) {
    case ParticleColorMode::RandomRGB:
        return { Rand01_(), Rand01_(), Rand01_(), params_.baseColor.w };

    case ParticleColorMode::RangeRGB:
    {
        float r = params_.baseColor.x + RandRange_(-params_.rgbRange.x, params_.rgbRange.x);
        float g = params_.baseColor.y + RandRange_(-params_.rgbRange.y, params_.rgbRange.y);
        float b = params_.baseColor.z + RandRange_(-params_.rgbRange.z, params_.rgbRange.z);
        return { Clamp01(r), Clamp01(g), Clamp01(b), params_.baseColor.w };
    }

    case ParticleColorMode::RangeHSV:
    {
        Vector3 hsv = params_.baseHSV;
        hsv.x += RandRange_(-params_.hsvRange.x, params_.hsvRange.x);
        hsv.y += RandRange_(-params_.hsvRange.y, params_.hsvRange.y);
        hsv.z += RandRange_(-params_.hsvRange.z, params_.hsvRange.z);

        hsv.x = std::fmod(hsv.x, 1.0f);
        if (hsv.x < 0.0f) hsv.x += 1.0f;

        hsv.y = Clamp01(hsv.y);
        hsv.z = Clamp01(hsv.z);

        Vector3 rgb = HSVtoRGB(hsv);
        return { Clamp01(rgb.x), Clamp01(rgb.y), Clamp01(rgb.z), params_.baseColor.w };
    }

    case ParticleColorMode::Fixed:
    default:
        return params_.baseColor;
    }
}

float ParticleEmitter::Rand01_() const {
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(GetRNG());
}

float ParticleEmitter::RandRange_(float minV, float maxV) const {
    return minV + (maxV - minV) * Rand01_();
}
