#include "Fullscreen.hlsli"

cbuffer GaussianFilterParam : register(b0) {
    int32_t gKernelRadius;
    float32_t gSigma;
    float32_t2 gDirection;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float gauss1D(float x, float sigma) {
    float exponent = -(x * x) / (2.0f * sigma * sigma);
    return exp(exponent); // 重みの合計で割るため、定数項は省略可能
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float32_t)width), rcp((float32_t)height));

    int32_t k = gKernelRadius;
    if (k < 1) k = 1;
    if (k > 10) k = 10;

    float32_t sigma = gSigma;
    if (sigma <= 0.0001f) sigma = 0.0001f;

    float32_t3 color = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t weight = 0.0f;

    for (int32_t i = -k; i <= k; ++i) {
        float32_t2 offset = gDirection * (float32_t)i * uvStepSize;
        float32_t2 texcoord = input.texcoord + offset;
        float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
        
        float32_t w = gauss1D((float32_t)i, sigma);
        color += fetchColor * w;
        weight += w;
    }

    output.color.rgb = color * rcp(weight);
    output.color.a = 1.0f;

    return output;
}
