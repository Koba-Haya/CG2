#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

struct RadialBlurParam {
    float32_t2 center;
    float32_t blurWidth;
    float32_t padding;
};

ConstantBuffer<RadialBlurParam> gParam : register(b0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    const int32_t kNumSamples = 10; // サンプリング数

    // 中心からの方向を計算
    float32_t2 direction = input.texcoord - gParam.center;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);

    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex) {
        float32_t2 texcoord = input.texcoord + direction * gParam.blurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSamplerLinear, texcoord).rgb;
    }

    // 平均化
    outputColor.rgb *= rcp(float32_t(kNumSamples));

    output.color.rgb = outputColor;
    output.color.a = 1.0f;

    return output;
}
