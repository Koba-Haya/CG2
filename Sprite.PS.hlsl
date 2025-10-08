// Sprite.PS.hlsl
#include "Sprite.hlsli"

float4 main(VSOut pin) : SV_TARGET
{
    float4 texColor = gTex.Sample(gSamp, pin.uv);
    return texColor * gColor;
}
