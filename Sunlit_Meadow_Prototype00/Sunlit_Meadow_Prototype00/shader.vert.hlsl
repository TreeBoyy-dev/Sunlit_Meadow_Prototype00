cbuffer UBO : register(b0, space1)
{
    column_major float4x4 mvp;
};

struct VSInput
{
    float3 position : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 color : TEXCOORD3;
    float inMaterialIndex : TEXCOORD4;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 out_normal : TEXCOORD0;
    float2 out_uv : TEXCOORD1;
    float4 out_color : TEXCOORD2;
    nointerpolation float fragMaterialIndex : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(mvp, float4(input.position, 1.0));
    output.out_normal = normalize(input.normal);
    output.out_uv = input.uv;
    output.out_color = input.color;
    output.fragMaterialIndex = input.inMaterialIndex;
    return output;
}
