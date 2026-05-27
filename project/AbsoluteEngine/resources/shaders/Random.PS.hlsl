#include "Fullscreen.hlsli"

// Randomパラメータ (b0にバインド)
cbuffer RandomParam : register(b0) {
    float32_t gTime;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

// 乱数生成関数 (0.0〜1.0未満)
float32_t rand2dTo1d(float32_t2 value) {
    return frac(sin(dot(value, float32_t2(12.9898f, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // シード値としてテクスチャ座標と時間を掛ける
    // gTimeが変化することで毎フレーム異なる乱数パターンが生成される
    float32_t random = rand2dTo1d(input.texcoord * gTime);
    
    // 元の画像の色を取得
    float32_t4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 乱数の値を入力画像に乗算する（RGB成分のみ）
    // randomの値をfloat32_t4の形にする
    output.color = float32_t4(baseColor.rgb * random, baseColor.a);
    
    return output;
}
