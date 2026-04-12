cbuffer ObjectConstants : register(b0)
{
  matrix worldViewProjection;
  matrix world;
  float4 color;
  float4 lightDirection;
};

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR0;
};

PSInput main(VSInput input)
{
  PSInput output;
  output.position = mul(float4(input.position, 1.0f), worldViewProjection);
  output.normal = mul(float4(input.normal, 0.0f), world).xyz;
  output.color = color;
  return output;
}
