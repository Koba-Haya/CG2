#include "object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 specularColor;
    float4x4 uvTransform;
    float shininess;
    float3 pad;
};

struct DirectionalLight
{
    float4 color; // ライトの色
    float3 direction; // ライトの向き
    float intensity; // 輝度
};

struct Camera
{
    float3 worldPosition;
    float pad;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // Cutout（必要なら）
    if (textureColor.a <= 0.5f)
    {
        discard;
    }

    float alpha = gMaterial.color.a * textureColor.a;
    if (alpha <= 0.0f)
    {
        discard;
    }

    float3 N = normalize(input.normal);
    float3 L = normalize(-gDirectionalLight.direction); // 面→光
    float NdotL = dot(N, L);

    float diffuseTerm = 1.0f;

    if (gMaterial.enableLighting == 1)
    {
        // Lambert
        diffuseTerm = saturate(NdotL);
    }
    else if (gMaterial.enableLighting == 2)
    {
        // Half Lambert
        diffuseTerm = pow(saturate(NdotL * 0.5f + 0.5f), 2.0f);
    }
    else
    {
        // Unlit
        diffuseTerm = 1.0f;
    }

    float3 diffuse =
        gMaterial.color.rgb *
        textureColor.rgb *
        diffuseTerm *
        gDirectionalLight.color.rgb *
        gDirectionalLight.intensity;

    float3 specular = float3(0.0f, 0.0f, 0.0f);

    // 鏡面反射（Phong）
    if (gMaterial.enableLighting != 0 && gMaterial.shininess > 0.0f)
    {
        float3 V = normalize(gCamera.worldPosition - input.worldPosition); // 面→視点
        float3 R = reflect(-L, N); // 入射は (光→面) なので -L

        float RdotV = saturate(dot(R, V));
        float specPow = pow(RdotV, gMaterial.shininess);

        specular =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            gMaterial.specularColor *
            specPow;
    }

    float3 finalRGB = diffuse;

    // Unlit のときは diffuse を組み直す（ライト無視）
    if (gMaterial.enableLighting == 0)
    {
        finalRGB = gMaterial.color.rgb * textureColor.rgb;
    }
    else
    {
        finalRGB = diffuse + specular;
    }

    output.color = float4(finalRGB, alpha);
    return output;
}
