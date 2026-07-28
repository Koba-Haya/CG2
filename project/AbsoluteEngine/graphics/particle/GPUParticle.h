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

// エミッタの形状（Sphere: 球内ランダム配置 / Box: 直方体内ランダム配置）
enum class GPUEmitterShape : uint32_t {
    Sphere = 0,
    Box = 1,
};

// 1エミッタ分のGPU側データ。HLSL側のEmitter(GPUParticle.hlsli)と1:1でレイアウトを合わせること。
// 16バイト境界をまたがないよう、64バイト(float4x4本分)ちょうどになるよう手動でパディングしている。
struct GPUEmitter {
    Vector3 translate;   // offset 0
    float radius;         // offset 12  (0-15: 16B)
    Vector3 halfExtents; // offset 16
    float pad0;           // offset 28  (16-31: 16B)
    uint32_t shape;        // offset 32
    uint32_t count;         // offset 36
    float frequency;       // offset 40
    float frequencyTime;   // offset 44 (32-47: 16B)
    uint32_t emit;          // offset 48
    uint32_t enabled;       // offset 52
    float pad1[2];          // offset 56 (48-63: 16B)
};

static constexpr uint32_t kMaxGPUEmitters = 8;

// b0 にバインドするConstantBuffer本体。8要素×64バイト=512バイトで16Bアライン済み。
struct GPUEmitterArray {
    GPUEmitter emitters[kMaxGPUEmitters];
};

// GPU Particleに適用するフィールド（CG3で学んだFieldのGPU Particle版）
enum class GPUFieldType : uint32_t {
    None = 0,
    Attractor = 1, // targetへ吸い寄せる
    Wind = 2,      // directionへ一定方向の力を加える
    Vortex = 3,    // direction軸を中心に渦を巻かせる
};

// 1Field分のデータ。こちらも32バイト(16Bブロック2つ)でレイアウトを固定する。
struct GPUField {
    Vector3 target;     // offset 0
    float strength;      // offset 12 (0-15: 16B)
    Vector3 direction;  // offset 16
    uint32_t type;        // offset 28 (16-31: 16B)
};

static constexpr uint32_t kMaxGPUFields = 4;

struct GPUFieldArray {
    GPUField fields[kMaxGPUFields]; // 4*32=128バイト
    uint32_t count;                  // offset 128
    float pad[3];                    // offset 132 (128-143: 16B)
};
