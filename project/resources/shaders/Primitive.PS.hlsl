#include "Primitive.hlsli"

float4 main(VertexShaderOutput input) : SV_TARGET
{
    // VSから渡された色をそのまま出力
    return input.color;
}