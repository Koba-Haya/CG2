#include "Fullscreen.hlsli"

Texture2D<float32_t4> gColorTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

struct DepthBasedOutlineParam {
    float32_t4x4 projectionInverse;
};
ConstantBuffer<DepthBasedOutlineParam> gParam : register(b0);

float32_t GetViewZ(float32_t ndcZ) {
    float32_t4 ndcPos = float32_t4(0.0f, 0.0f, ndcZ, 1.0f);
    float32_t4 viewPos = mul(ndcPos, gParam.projectionInverse);
    return viewPos.z / viewPos.w;
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint32_t width, height;
    gDepthTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float32_t)width), rcp((float32_t)height));

    float32_t prewittHorizontal[3][3] = {
        {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
        {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
        {-1.0f / 6.0f, 0.0f, 1.0f / 6.0f}
    };
    float32_t prewittVertical[3][3] = {
        {-1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f},
        { 0.0f,         0.0f,         0.0f},
        { 1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f}
    };

    float32_t weightX = 0.0f;
    float32_t weightY = 0.0f;

    for(int32_t x = 0; x < 3; ++x) {
        for(int32_t y = 0; y < 3; ++y) {
            float32_t2 texcoord = input.texcoord + float32_t2(x - 1, y - 1) * uvStepSize;
            float32_t ndcZ = gDepthTexture.Sample(gSampler, texcoord);
            float32_t viewZ = GetViewZ(ndcZ);
            
            weightX += viewZ * prewittHorizontal[y][x];
            weightY += viewZ * prewittVertical[y][x];
        }
    }

    float32_t weight = length(float32_t2(weightX, weightY));
    
    // 微小な変化（床の傾きなど）を無視する
    float32_t threshold = 0.1f; 
    weight = saturate((weight - threshold) * 5.0f);
    
    // 元の画像をサンプリング
    float32_t4 baseColor = gColorTexture.Sample(gSampler, input.texcoord);
    
    // 輪郭線の色（今回は黒）
    float32_t3 outlineColor = float32_t3(0.0f, 0.0f, 0.0f);
    
    // エッジの重みに応じて、元の色と輪郭線の色をブレンドする
    output.color.rgb = lerp(baseColor.rgb, outlineColor, weight);
    output.color.a = baseColor.a;
    
    return output;
}
