#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct HSVParam {
    float32_t hue;
    float32_t saturation;
    float32_t value;
    float32_t pad;
};
ConstantBuffer<HSVParam> gHSVParam : register(b0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

float32_t WrapValue(float32_t value, float32_t minRange, float32_t maxRange) {
    float32_t range = maxRange - minRange;
    float32_t modValue = fmod(value - minRange, range);
    if (modValue < 0.0f) {
        modValue += range;
    }
    return minRange + modValue;
}

float32_t3 RGBToHSV(float32_t3 c) {
    float32_t4 K = float32_t4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float32_t4 p = lerp(float32_t4(c.bg, K.wz), float32_t4(c.gb, K.xy), step(c.b, c.g));
    float32_t4 q = lerp(float32_t4(p.xyw, c.r), float32_t4(c.r, p.yzx), step(p.x, c.r));

    float32_t d = q.x - min(q.w, q.y);
    float32_t e = 1.0e-10f;
    return float32_t3(abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}

float32_t3 HSVToRGB(float32_t3 c) {
    float32_t4 K = float32_t4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float32_t3 p = abs(frac(c.xxx + K.xyz) * 6.0f - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // RGBをHSVに変換
    float32_t3 hsv = RGBToHSV(textureColor.rgb);
    
    // パラメータによる調整
    hsv.x += gHSVParam.hue;
    hsv.y += gHSVParam.saturation;
    hsv.z += gHSVParam.value;
    
    // 色相は0～1の範囲でラップ、彩度と明度は0～1にクランプ
    hsv.x = WrapValue(hsv.x, 0.0f, 1.0f);
    hsv.y = saturate(hsv.y);
    hsv.z = saturate(hsv.z);
    
    // HSVをRGBに戻す
    float32_t3 rgb = HSVToRGB(hsv);
    
    output.color.rgb = rgb;
    output.color.a = textureColor.a;
    
    return output;
}
