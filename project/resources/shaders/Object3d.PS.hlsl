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

struct Camera
{
    float3 worldPosition;
    float pad;
};

struct DirectionalLight
{
    float4 color; // ライトの色
    float3 direction; // ライトの向き
    float intensity; // 輝度
    int enabled; // 0:OFF 1:ON
};

struct PointLight
{
    float4 color; // ライトの色
    float3 position; // ライトの向き
    float intensity; // 輝度
    float radius; // 光の届く最大距離
    float decay; // 減衰率
    int enabled; // 0:OFF 1:ON
};

struct SpotLight
{
    float4 color; // ライトの色
    float3 position; // ライトの位置
    float intensity; // 輝度
    float3 direction; // ライトの向き
    float distance; // 光の届く最大距離
    float decay; // 減衰率
    float cosAngle; // 内側の角度
    int enabled; // 0:OFF 1:ON
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a <= 0.5f)
    {
        discard;
    }

    float alpha = gMaterial.color.a * textureColor.a;
    if (alpha <= 0.0f)
    {
        discard;
    }

    // Unlit
    if (gMaterial.enableLighting == 0)
    {
        output.color = float4(gMaterial.color.rgb * textureColor.rgb, alpha);
        return output;
    }
    
    // 0 or 1 にして掛け算しやすくする
    float dirOn = (gDirectionalLight.enabled != 0) ? 1.0f : 0.0f;
    float pointOn = (gPointLight.enabled != 0) ? 1.0f : 0.0f;
    float spotOn = (gSpotLight.enabled != 0) ? 1.0f : 0.0f;
    
    float3 N = normalize(input.normal);

    // 面→視点
    float3 V = normalize(gCamera.worldPosition - input.worldPosition);

    // ===== DirectionalLight =====
    float3 Ld = normalize(-gDirectionalLight.direction); // 面→光
    float NdotLd = dot(N, Ld);

    float diffuseTermD = 1.0f;
    if (gMaterial.enableLighting == 1)
    {
        diffuseTermD = saturate(NdotLd);
    }
    else
    {
        diffuseTermD = pow(saturate(NdotLd * 0.5f + 0.5f), 2.0f);
    }

    float3 diffuseDirectional =
        gMaterial.color.rgb *
        textureColor.rgb *
        diffuseTermD *
        gDirectionalLight.color.rgb *
        gDirectionalLight.intensity * dirOn;

    float3 specularDirectional = float3(0.0f, 0.0f, 0.0f);
    if (gMaterial.shininess > 0.0f)
    {
        float3 Hd = normalize(Ld + V);
        float NdotHd = saturate(dot(N, Hd));
        float specPowD = pow(NdotHd, gMaterial.shininess);

        specularDirectional =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            gMaterial.specularColor *
            specPowD * dirOn;
    }

    // ===== PointLight =====
    float3 pointLightDirection = normalize(gPointLight.position - input.worldPosition); // 面→点光源
    float NdotPointLightDirection = dot(N, pointLightDirection);
    float poinDistance = length(gPointLight.position - input.worldPosition); // ポイントライトへの距離
    float pointFactor = pow(saturate(-poinDistance / gPointLight.radius + 1.0f), gPointLight.decay); // 指数による距離減衰

    float diffuseTermP = 1.0f;
    if (gMaterial.enableLighting == 1)
    {
        diffuseTermP = saturate(NdotPointLightDirection);
    }
    else
    {
        diffuseTermP = pow(saturate(NdotPointLightDirection * 0.5f + 0.5f), 2.0f);
    }

    float3 diffusePoint =
        gMaterial.color.rgb *
        textureColor.rgb *
        diffuseTermP *
        gPointLight.color.rgb *
        gPointLight.intensity * pointFactor * pointOn;

    float3 specularPoint = float3(0.0f, 0.0f, 0.0f);
    if (gMaterial.shininess > 0.0f)
    {
        float3 Hp = normalize(pointLightDirection + V);
        float NdotHp = saturate(dot(N, Hp));
        float specPowP = pow(NdotHp, gMaterial.shininess);

        specularPoint =
            gPointLight.color.rgb *
            gPointLight.intensity *
            gMaterial.specularColor *
            specPowP * pointFactor * pointOn;
    }
    
        // ===== SpotLight =====
    float3 Ls = normalize(gSpotLight.position - input.worldPosition);// 面→光
    float NdotLs = dot(N, Ls);

    // 距離減衰
    float spotDistance = length(gSpotLight.position - input.worldPosition);
    float attenuationSpot = pow(saturate(-spotDistance / gSpotLight.distance + 1.0f), gSpotLight.decay);

    // Falloff（スポットの中心1、外側0）
    float3 lightToSurfaceDir = normalize(input.worldPosition - gSpotLight.position); // ライト→面
    float cosTheta = dot(lightToSurfaceDir, normalize(gSpotLight.direction)); // 中心軸との角度cos

    float falloff = saturate((cosTheta - gSpotLight.cosAngle) / (1.0f - gSpotLight.cosAngle));

    float diffuseTermS = 1.0f;
    if (gMaterial.enableLighting == 1)
    {
        diffuseTermS = saturate(NdotLs);
    }
    else
    {
        diffuseTermS = pow(saturate(NdotLs * 0.5f + 0.5f), 2.0f);
    }

    float3 diffuseSpot =
        gMaterial.color.rgb *
        textureColor.rgb *
        diffuseTermS *
        gSpotLight.color.rgb *
        gSpotLight.intensity *
        attenuationSpot *
        falloff *
        spotOn;

    float3 specularSpot = float3(0.0f, 0.0f, 0.0f);
    if (gMaterial.shininess > 0.0f)
    {
        float3 Hs = normalize(Ls + V);
        float NdotHs = saturate(dot(N, Hs));
        float specPowS = pow(NdotHs, gMaterial.shininess);

        specularSpot =
            gSpotLight.color.rgb *
            gSpotLight.intensity *
            gMaterial.specularColor *
            specPowS *
            attenuationSpot *
            falloff *
            spotOn;
    }

    float3 finalRGB =
        diffuseDirectional + specularDirectional +
        diffusePoint + specularPoint +
        diffuseSpot + specularSpot;
    output.color = float4(finalRGB, alpha);
    return output;
}