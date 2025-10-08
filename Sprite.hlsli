cbuffer TransformVS : register(b0)
{
    row_major float4x4 gWVP;
};

cbuffer MaterialPS : register(b0)
{
    float4 gColor;
};

Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct VSIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};