#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct DissolveParam {
    float32_t threshold;
    float32_t edgeRange;
    float32_t2 padding; // 16byte alignment
    float32_t3 edgeColor;
    float32_t padding2; // 16byte alignment
    float32_t3 maskColor;
    float32_t padding3; // 16byte alignment
};
ConstantBuffer<DissolveParam> gDissolve : register(b0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord).r;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    
    float32_t3 resultColor = texColor.rgb;
    float32_t resultAlpha = texColor.a;

    if (mask <= gDissolve.threshold) {
        // 完全に溶けた部分は maskColor で塗りつぶす
        resultColor = gDissolve.maskColor;
        resultAlpha = 1.0f;
    } else if (mask <= gDissolve.threshold + gDissolve.edgeRange) {
        // エッジ部分は edgeColor を乗せる
        // edgeWeight: 0.0(溶けている境界) ～ 1.0(元の絵の境界)
        float32_t edgeWeight = (mask - gDissolve.threshold) / gDissolve.edgeRange;
        
        // エッジの色を強く出すため、edgeWeight が小さいほど edgeColor に近づく
        float32_t mixFactor = 1.0f - edgeWeight;
        
        // 元の絵とエッジカラーをブレンド
        resultColor = lerp(texColor.rgb, gDissolve.edgeColor, mixFactor);
    }
    
    output.color = float32_t4(resultColor, resultAlpha);
    
    return output;
}
