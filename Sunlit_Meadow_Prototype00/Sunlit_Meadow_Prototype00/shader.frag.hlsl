Texture2DArray tex_sampler : register(t0, space2);
SamplerState tex_sampler_state : register(s0, space2);

struct PSInput
{
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    nointerpolation float fragMaterialIndex : TEXCOORD2;
};

float4 main(PSInput input) : SV_Target0
{
    return tex_sampler.Sample(tex_sampler_state, float3(input.uv, input.fragMaterialIndex)) * input.color;
}
