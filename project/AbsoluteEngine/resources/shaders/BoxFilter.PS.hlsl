#include "Fullscreen.hlsli"

cbuffer BoxFilterParam : register(b0) {
    int32_t gKernelRadius;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float32_t)width), rcp((float32_t)height));

    int32_t k = gKernelRadius;
    if (k < 1) k = 1; // 最小は1 (3x3)
    if (k > 10) k = 10; // 制限（ループ数が大きくなりすぎないように）

    float32_t3 color = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t weight = 1.0f / ((2.0f * k + 1.0f) * (2.0f * k + 1.0f));

    for (int32_t x = -k; x <= k; ++x) {
        for (int32_t y = -k; y <= k; ++y) {
            float32_t2 texcoord = input.texcoord + float32_t2(x, y) * uvStepSize;
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            color += fetchColor * weight;
        }
    }

    output.color.rgb = color;
    output.color.a = 1.0f;

    return output;
}
