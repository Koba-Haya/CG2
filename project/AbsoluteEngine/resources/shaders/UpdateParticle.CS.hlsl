#include "GPUParticle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<FieldArray> gFields : register(b2);
RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<uint32_t> gFreeList : register(u2);

static const uint32_t kMaxParticles = 1024;

// CG3で学んだFieldの概念をGPU Particleに適用する。
// Attractor: targetへ吸い寄せる / Wind: directionへ一定の力 / Vortex: direction軸周りに渦を巻かせる
float32_t3 ApplyFields(float32_t3 position) {
    float32_t3 accel = float32_t3(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < gFields.count; ++i) {
        Field f = gFields.fields[i];
        if (f.type == 1) {
            // Attractor
            float32_t3 toTarget = f.target - position;
            float32_t dist = length(toTarget);
            if (dist > 0.0001f) {
                accel += (toTarget / dist) * f.strength;
            }
        } else if (f.type == 2) {
            // Wind
            float32_t3 dir = f.direction;
            float32_t len = length(dir);
            if (len > 0.0001f) {
                accel += (dir / len) * f.strength;
            }
        } else if (f.type == 3) {
            // Vortex: direction軸を中心にtargetからの相対位置に接線方向の力を与える
            float32_t3 axis = f.direction;
            float32_t axisLen = length(axis);
            if (axisLen > 0.0001f) {
                axis /= axisLen;
                float32_t3 toParticle = position - f.target;
                float32_t3 tangent = cross(axis, toParticle);
                float32_t tangentLen = length(tangent);
                if (tangentLen > 0.0001f) {
                    accel += (tangent / tangentLen) * f.strength;
                }
            }
        }
    }
    return accel;
}

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles) {
        // alphaが0のparticleは死んでいるとみなして更新しない
        if (gParticles[particleIndex].color.a != 0.0f) {
            // Fieldによる加速度を速度に積算する
            float32_t3 accel = ApplyFields(gParticles[particleIndex].translate);
            gParticles[particleIndex].velocity += accel * gPerFrame.deltaTime;

            // 移動処理（フレームレートに依存しないよう deltaTime を乗算）
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;

            // 寿命進行処理
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;

            // アルファ値（フェードアウト）の計算
            float32_t alpha = 1.0f - (gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
            gParticles[particleIndex].color.a = saturate(alpha);

            // alphaが0になった（寿命を迎えた）らFreeListに戻す
            if (gParticles[particleIndex].color.a == 0.0f) {
                // スケールを0にして頂点シェーダ側で描画を無効化
                gParticles[particleIndex].scale = float32_t3(0.0f, 0.0f, 0.0f);

                int32_t freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);

                // 最新のFreeListIndexの場所に死んだParticleのIndexを設定する
                if ((freeListIndex + 1) < kMaxParticles) {
                    gFreeList[freeListIndex + 1] = particleIndex;
                } else {
                    // 通常到達しない安全策
                    InterlockedAdd(gFreeListIndex[0], -1);
                }
            }
        }
    }
}
