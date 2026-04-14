cbuffer GridConstants : register(b0)
{
  matrix worldViewProjection;
  matrix world;
  float4 minorColor;
  float4 majorColor;
  float4 gridParams;
  float4 cameraWorldPosition;
};

struct VSInput
{
  float3 position : POSITION;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 worldPosition : TEXCOORD0;
};

PSInput main(VSInput input)
{
  PSInput output;
  float4 worldPosition = mul(float4(input.position, 1.0f), world);
  output.position = mul(float4(input.position, 1.0f), worldViewProjection);
  output.worldPosition = worldPosition.xyz;
  return output;
}
