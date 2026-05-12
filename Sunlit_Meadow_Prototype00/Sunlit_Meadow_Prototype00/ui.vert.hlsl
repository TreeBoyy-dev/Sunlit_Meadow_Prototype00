struct VSInput
{
	float2 pos : TEXCOORD0;
	float4 color : TEXCOORD1;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.pos = float4(input.pos, 0.0, 1.0);
	output.color = input.color;
	return output;
}