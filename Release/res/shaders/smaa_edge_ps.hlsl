cbuffer PostProcessConstants : register(b0)
{
  float2 texelSize;
  float2 screenSize;
  float4 parameters;
};

Texture2D sceneTexture : register(t0);
SamplerState linearClampSampler : register(s0);

struct PSInput
{
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float Luma(float3 color)
{
  return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float4 main(PSInput input) : SV_TARGET
{
  float lumaCenter = Luma(sceneTexture.Sample(linearClampSampler, input.uv).rgb);
  float lumaLeft = Luma(sceneTexture.Sample(linearClampSampler, input.uv + float2(-texelSize.x, 0.0f)).rgb);
  float lumaRight = Luma(sceneTexture.Sample(linearClampSampler, input.uv + float2(texelSize.x, 0.0f)).rgb);
  float lumaUp = Luma(sceneTexture.Sample(linearClampSampler, input.uv + float2(0.0f, -texelSize.y)).rgb);
  float lumaDown = Luma(sceneTexture.Sample(linearClampSampler, input.uv + float2(0.0f, texelSize.y)).rgb);

  float horizontal = saturate(max(abs(lumaLeft - lumaCenter), abs(lumaRight - lumaCenter)) / max(parameters.x, 1.0e-4f));
  float vertical = saturate(max(abs(lumaUp - lumaCenter), abs(lumaDown - lumaCenter)) / max(parameters.x, 1.0e-4f));

  return float4(horizontal, vertical, max(horizontal, vertical), 1.0f);
}
