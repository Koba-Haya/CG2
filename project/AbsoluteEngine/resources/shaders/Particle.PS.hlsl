#include "Particle.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 tex = gTexture.Sample(gSampler, uv.xy);
    float4 col = input.color * gMaterial.color * tex;

    if (col.a <= 0.0f)
        discard;

    output.color = col;
    return output;
}