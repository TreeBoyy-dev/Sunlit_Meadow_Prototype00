[[vk::combinedImageSampler]][[vk::binding(0, 2)]] Texture2D tex : register(t0);
[[vk::combinedImageSampler]][[vk::binding(0, 2)]] SamplerState samp : register(s0);

struct PSInput
{
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target
{
    return tex.Sample(samp, input.uv) * input.color;
}