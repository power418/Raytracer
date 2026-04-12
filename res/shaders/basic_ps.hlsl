cbuffer ObjectConstants : register(b0)
{
  matrix worldViewProjection;
  matrix world;
  float4 color;
  float4 lightDirection;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
  float3 normal = normalize(input.normal);
  float diffuse = saturate(dot(normal, normalize(-lightDirection.xyz)));
  float lighting = 0.25f + diffuse * 0.75f;
  return float4(input.color.rgb * lighting, input.color.a);
}
