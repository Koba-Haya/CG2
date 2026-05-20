#include "GPUParticle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<uint32_t> gFreeList : register(u2);

static const uint32_t kMaxParticles = 1024;

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles) {
        // alphaが0のparticleは死んでいるとみなして更新しない
        if (gParticles[particleIndex].color.a != 0.0f) {
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
