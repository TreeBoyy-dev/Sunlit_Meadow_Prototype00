TextureCube skyboxTex : register(t0, space2);
SamplerState skyboxSampler : register(s0, space2);

struct PSInput
{
    float3 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
    return skyboxTex.Sample(skyboxSampler, input.texCoord);
}