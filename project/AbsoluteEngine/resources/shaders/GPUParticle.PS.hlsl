Texture2D<float32_t4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderInput {
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD;
    float32_t4 color : COLOR;
};

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input) {
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // テクスチャのRチャネル（輝度）をアルファ値として適用
    float32_t alpha = textureColor.r * input.color.a;
    
    // 出力色を決定（テクスチャのRGBと入力色を乗算、アルファには上記で求めた値を使用）
    output.color = float32_t4(textureColor.rgb * input.color.rgb, alpha);
    
    // アルファがほぼ0ならピクセルを破棄
    if (output.color.a <= 0.005f) {
        discard;
    }
    return output;
}
