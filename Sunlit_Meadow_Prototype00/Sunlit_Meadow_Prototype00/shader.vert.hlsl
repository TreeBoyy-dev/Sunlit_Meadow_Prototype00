cbuffer UBO : register(b0, space1)
{
    column_major float4x4 mvp;
};

struct VSInput
{
    float3 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
    float inMaterialIndex : TEXCOORD3;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 out_uv : TEXCOORD0;
    float4 out_color : TEXCOORD1;
    nointerpolation float fragMaterialIndex : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(mvp, float4(input.position, 1.0));
    output.out_color = input.color;
    output.out_uv = input.uv;
    output.fragMaterialIndex = input.inMaterialIndex;
    return output;
}
