#include "Primitive.hlsli"

// 定数バッファの定義（cbuffer構文に変更）
cbuffer TransformationMatrix : register(b0)
{
    float4x4 WVP;
};

struct VertexShaderInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // 3D空間の座標をスクリーン座標に変換
    output.position = mul(float4(input.position, 1.0f), WVP);
    output.color = input.color;
    return output;
}