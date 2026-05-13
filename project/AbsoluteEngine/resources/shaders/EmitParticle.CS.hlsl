#include "GPUParticle.hlsli"

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

static const uint32_t kMaxParticles = 1024;

[numthreads(1, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    if (gEmitter.emit != 0) {
        RandomGenerator generator;
        // シード値の設定（スレッドIDと時間を組み合わせる）
        generator.seed = (float32_t3(DTid) + gPerFrame.time) * gPerFrame.time;
        
        for (uint32_t countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
            int32_t particleIndex;
            // アトミック加算で書き込み先のインデックスを取得
            InterlockedAdd(gFreeCounter[0], 1, particleIndex);
            
            if (particleIndex < kMaxParticles) {
                // パーティクルの初期化
                gParticles[particleIndex].scale = generator.Generate3d() * 0.3f + 0.1f;
                
                // エミッターの範囲（球状）内にランダム配置
                float32_t3 randPos = generator.Generate3d() * 2.0f - 1.0f;
                gParticles[particleIndex].translate = gEmitter.translate + randPos * gEmitter.radius;
                
                gParticles[particleIndex].color.rgb = generator.Generate3d();
                gParticles[particleIndex].color.a = 1.0f;
                
                // ランダムな速度と寿命
                gParticles[particleIndex].velocity = (generator.Generate3d() * 2.0f - 1.0f) * 2.0f;
                gParticles[particleIndex].lifeTime = 1.0f + generator.Generate1d() * 2.0f;
                gParticles[particleIndex].currentTime = 0.0f;
            }
        }
    }
}
