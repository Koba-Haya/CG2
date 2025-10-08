// Sprite.VS.hlsl
#include "Sprite.hlsli"

VSOut main(VSIn vin)
{
    VSOut vout;
    vout.svpos = mul(float4(vin.pos, 1.0f), gWVP);
    vout.uv = vin.uv;
    return vout;
}
