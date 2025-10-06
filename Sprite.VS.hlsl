cbuffer TransformVS : register(b0)
{
    row_major float4x4 gWVP;
};

struct VSIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0; // ← 明示的に TEXCOORD0 にする
};

VSOut main(VSIn vin)
{
    VSOut vout;
    float4 p = float4(vin.pos, 1.0f);
    vout.svpos = mul(p, gWVP);
    vout.uv = vin.uv;
    return vout;
}
