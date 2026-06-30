Texture2DArray tex_sampler : register(t0, space2);
SamplerState tex_sampler_state : register(s0, space2);

struct PSInput
{
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
    nointerpolation float fragMaterialIndex : TEXCOORD3;
};

float4 main(PSInput input) : SV_Target0
{
    // Simple directional light from above and slightly to the front
    float3 lightDir = normalize(float3(0.4, 0.3, 1.0));
    float diffuse = saturate(dot(normalize(input.normal), lightDir));
    float ambient = 0.55;
    float lighting = ambient + (1.0 - ambient) * diffuse;

    float4 texColor = tex_sampler.Sample(tex_sampler_state, float3(input.uv, input.fragMaterialIndex));

    if (texColor.a < 0.5) discard;

    float3 rgb = texColor.rgb * input.color.rgb * lighting;
    float a = texColor.a * input.color.a;
    return float4(rgb, a);
}
