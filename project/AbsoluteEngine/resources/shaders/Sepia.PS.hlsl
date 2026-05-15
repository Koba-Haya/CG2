#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // BT.709 輝度計算
    float32_t gray = dot(output.color.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    
    // セピア色への変換 (RGB: 107, 74, 43 をベースに比率計算)
    output.color.rgb = gray * float32_t3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);
    
    return output;
}
