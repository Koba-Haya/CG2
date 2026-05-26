#include "Fullscreen.hlsli"

// Vignette パラメータ
cbuffer VignetteParam : register(b0) {
    float32_t gScale;
    float32_t gPowValue;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // 周囲を0に、中心になるほど明るくなるように計算で調整
    float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // correctだけで計算すると中心の最大値が0.0625で暗すぎるのでScaleで調整
    float32_t vignette = correct.x * correct.y * gScale;
    
    // とりあえず指定乗数でそれっぽくする
    vignette = saturate(pow(vignette, gPowValue));
    
    // 係数として乗算
    output.color.rgb *= vignette;
    
    return output;
}
