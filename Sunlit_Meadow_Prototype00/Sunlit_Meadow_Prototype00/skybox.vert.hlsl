cbuffer UBO : register(b0, space1)
{
    column_major float4x4 mvp;
};

struct VSInput
{
    float3 inPos : TEXCOORD0;
};

struct VSOutput
{
    float3 texCoord : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.texCoord = float3(-input.inPos.y, input.inPos.z, input.inPos.x);

    float4 pos = mul(mvp, float4(input.inPos, 1.0));
    output.position = pos.xyww;

    return output;
}