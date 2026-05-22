Texture2D tex_sampler : register(t0, space2);
SamplerState tex_sampler_state : register(s0, space2);

struct PSInput
{
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
};

float4 main(PSInput input) : SV_Target0
{
    // Same simple directional light as the world shader, so mobs are lit
    // consistently with the terrain.
    float3 lightDir = normalize(float3(0.4, 0.3, 1.0));
    float diffuse = saturate(dot(normalize(input.normal), lightDir));
    float ambient = 0.55;
    float lighting = ambient + (1.0 - ambient) * diffuse;

    float4 texColor = tex_sampler.Sample(tex_sampler_state, input.uv);
    return texColor * input.color * lighting;
}
