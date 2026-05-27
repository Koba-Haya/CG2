#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t kPrewittHorizontalKernel[3][3] = {
    {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
    {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
    {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
};
static const float32_t kPrewittVerticalKernel[3][3] = {
    {-1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f},
    { 0.0f,         0.0f,         0.0f},
    { 1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f},
};

float32_t Luminance(float32_t3 v) {
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float32_t)width), rcp((float32_t)height));

    float32_t weightX = 0.0f;
    float32_t weightY = 0.0f;

    for (int32_t x = 0; x < 3; ++x) {
        for (int32_t y = 0; y < 3; ++y) {
            float32_t2 texcoord = input.texcoord + float32_t2(x - 1, y - 1) * uvStepSize;
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            float32_t lum = Luminance(fetchColor);

            weightX += lum * kPrewittHorizontalKernel[y][x];
            weightY += lum * kPrewittVerticalKernel[y][x];
        }
    }

    float32_t weight = length(float32_t2(weightX, weightY));
    
    // 微小な変化を無視する（輝度差の閾値）
    float32_t threshold = 0.05f;
    weight = saturate((weight - threshold) * 10.0f);

    // 元の画像をサンプリング
    float32_t4 baseColor = gTexture.Sample(gSampler, input.texcoord);

    // 輪郭線の色（黒）
    float32_t3 outlineColor = float32_t3(0.0f, 0.0f, 0.0f);
    
    // 元の色に輪郭線を合成
    output.color.rgb = lerp(baseColor.rgb, outlineColor, weight);
    output.color.a = baseColor.a;

    return output;
}
