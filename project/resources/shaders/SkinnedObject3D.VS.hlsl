#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct SkinningData {
    float4x4 boneMatrices[128];
};
ConstantBuffer<SkinningData> gSkin : register(b3);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    uint4 boneIDs : BONEIDS;
    float4 weights : WEIGHTS;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // Skinning calculation
    float4x4 skinMatrix = 
        input.weights.x * gSkin.boneMatrices[input.boneIDs.x] +
        input.weights.y * gSkin.boneMatrices[input.boneIDs.y] +
        input.weights.z * gSkin.boneMatrices[input.boneIDs.z] +
        input.weights.w * gSkin.boneMatrices[input.boneIDs.w];

    float4 skinnedPosition = mul(input.position, skinMatrix);
    skinnedPosition.w = 1.0f;
    float3 skinnedNormal = normalize(mul(input.normal, (float3x3)skinMatrix));

    output.position = mul(skinnedPosition, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    
    output.normal = normalize(mul(skinnedNormal, (float3x3) gTransformationMatrix.WorldInverseTranspose));

    float4 worldPos4 = mul(skinnedPosition, gTransformationMatrix.World);
    output.worldPosition = worldPos4.xyz;

    return output;
}
