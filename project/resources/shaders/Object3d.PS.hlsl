#include "object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct DirectionalLight
{
    float4 color; // ライトの色
    float3 direction; // ライトの向き
    float intensity; // 輝度
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float NdotL = dot(normal, lightDir);
    float lighting = 1.0f;

    if (gMaterial.enableLighting == 1)
    {
        // Lambert
        lighting = saturate(NdotL);
    }
    else if (gMaterial.enableLighting == 2)
    {
        // Half Lambert
        lighting = pow(NdotL * 0.5f + 0.5f, 2.0f);
    }
    else
    {
        // No Lighting
        lighting = 1.0f;
    }

    float3 finalColor = gMaterial.color.rgb * textureColor.rgb * lighting * gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    
    if (gMaterial.enableLighting == 0)
    {
    // 完全にアンリット（照明無視）
        finalColor = gMaterial.color.rgb * textureColor.rgb;
    }
    else
    {
        finalColor.rgb = gMaterial.color.rgb * textureColor.rgb * lighting * gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    }
    
    output.color = float4(finalColor, 1.0f);
    return output;
}
