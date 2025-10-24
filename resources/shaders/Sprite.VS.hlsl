#include "Sprite.hlsli"

cbuffer TransformVS : register(b0)
{
    row_major float4x4 gWVP;
}

VSOut main(VSIn vin)
{
    VSOut o;
    o.svpos = mul(float4(vin.pos, 1), gWVP);
    o.uv = vin.uv;
    return o;
}
