cbuffer UBO : register(b0, space1)
{
    column_major float4x4 mvp;
};

// Must match the engine Vertex layout / pipeline attributes (locations 0..4).
// materialIndex (TEXCOORD4) is unused by entities but kept so the input layout
// stays identical to the world pipeline's Vertex.
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
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(mvp, float4(input.position, 1.0));
    output.out_normal = normalize(input.normal);
    output.out_uv = input.uv;
    output.out_color = input.color;
    return output;
}
