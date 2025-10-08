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