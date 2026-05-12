#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct WellForGPU {
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};
StructuredBuffer<WellForGPU> gMatrixPalette : register(t2);

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
        input.weights.x * gMatrixPalette[input.boneIDs.x].skeletonSpaceMatrix +
        input.weights.y * gMatrixPalette[input.boneIDs.y].skeletonSpaceMatrix +
        input.weights.z * gMatrixPalette[input.boneIDs.z].skeletonSpaceMatrix +
        input.weights.w * gMatrixPalette[input.boneIDs.w].skeletonSpaceMatrix;

    float3x3 skinNormalMatrix = 
        input.weights.x * (float3x3)gMatrixPalette[input.boneIDs.x].skeletonSpaceInverseTransposeMatrix +
        input.weights.y * (float3x3)gMatrixPalette[input.boneIDs.y].skeletonSpaceInverseTransposeMatrix +
        input.weights.z * (float3x3)gMatrixPalette[input.boneIDs.z].skeletonSpaceInverseTransposeMatrix +
        input.weights.w * (float3x3)gMatrixPalette[input.boneIDs.w].skeletonSpaceInverseTransposeMatrix;

    float4 skinnedPosition = mul(input.position, skinMatrix);
    skinnedPosition.w = 1.0f;
    float3 skinnedNormal = normalize(mul(input.normal, skinNormalMatrix));

    output.position = mul(skinnedPosition, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    
    output.normal = normalize(mul(skinnedNormal, (float3x3) gTransformationMatrix.WorldInverseTranspose));

    float4 worldPos4 = mul(skinnedPosition, gTransformationMatrix.World);
    output.worldPosition = worldPos4.xyz;

    return output;
}
