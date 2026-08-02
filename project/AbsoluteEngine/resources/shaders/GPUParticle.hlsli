struct GPUParticle {
    float32_t3 translate;
    float32_t3 scale;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
};

// GPUEmitter(GPUParticle.h)とレイアウトを1:1で一致させること
struct Emitter {
    float32_t3 translate;
    float32_t radius;
    float32_t3 halfExtents;
    float32_t pad0;
    uint32_t shape; // 0:Sphere 1:Box
    uint32_t count;
    float32_t frequency;
    float32_t frequencyTime;
    uint32_t emit;
    uint32_t enabled;
    float32_t2 pad1;
};

static const uint32_t kMaxEmitters = 8;

struct EmitterArray {
    Emitter emitters[kMaxEmitters];
};

// GPUField(GPUParticle.h)とレイアウトを1:1で一致させること
struct Field {
    float32_t3 target;
    float32_t strength;
    float32_t3 direction;
    uint32_t type; // 0:None 1:Attractor 2:Wind 3:Vortex
};

static const uint32_t kMaxFields = 4;

struct FieldArray {
    Field fields[kMaxFields];
    uint32_t count;
};

struct PerFrame {
    float32_t time;
    float32_t deltaTime;
};

float32_t3 rand3dTo3d(float32_t3 value) {
    float32_t3 s = float32_t3(
        dot(value, float32_t3(127.1, 311.7, 74.7)),
        dot(value, float32_t3(269.5, 183.3, 246.1)),
        dot(value, float32_t3(113.5, 271.9, 124.6))
    );
    return frac(sin(s) * 43758.5453123);
}

float32_t rand3dTo1d(float32_t3 value) {
    return frac(sin(dot(value, float32_t3(12.9898, 78.233, 45.164))) * 43758.5453123);
}

class RandomGenerator {
    float32_t3 seed;
    float32_t3 Generate3d() {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float32_t Generate1d() {
        float32_t result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
};
