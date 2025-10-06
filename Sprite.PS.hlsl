cbuffer MaterialPS : register(b0)
{
    float4 gColor; // 乗算色（1,1,1,1 で無変化）
};

Texture2D gTex : register(t0); // UnifiedPipeline の静的サンプラ s0 を使う
SamplerState gSamp : register(s0);

float4 main(float2 uv : TEXCOORD0) : SV_TARGET // ← TEXCOORD0 に合わせる
{
    float4 tex = gTex.Sample(gSamp, uv);
    return tex * gColor;
}