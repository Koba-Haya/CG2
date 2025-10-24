#include "Sprite.hlsli"

// C++側の並びと完全一致させる（colorの直後に行列）
cbuffer MaterialPS : register(b0)
{
    float4 gColor;
    row_major float4x4 gUV; // 行優先でCPUの行列と一致
}

Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

float4 main(VSOut pin) : SV_TARGET
{
    float4 uvh = mul(float4(pin.uv, 0, 1), gUV);
    return gTex.Sample(gSamp, uvh.xy) * gColor;
}
