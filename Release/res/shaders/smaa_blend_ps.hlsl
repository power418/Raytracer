cbuffer PostProcessConstants : register(b0)
{
  float2 texelSize;
  float2 screenSize;
  float4 parameters;
};

Texture2D sceneTexture : register(t0);
Texture2D edgeTexture : register(t1);
SamplerState linearClampSampler : register(s0);

struct PSInput
{
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
  float4 baseColor = sceneTexture.Sample(linearClampSampler, input.uv);
  float2 edge = edgeTexture.Sample(linearClampSampler, input.uv).rg;

  if (max(edge.x, edge.y) < 0.01f)
  {
    return baseColor;
  }

  float4 horizontalBlend = 0.5f * (
    sceneTexture.Sample(linearClampSampler, input.uv + float2(-texelSize.x, 0.0f)) +
    sceneTexture.Sample(linearClampSampler, input.uv + float2(texelSize.x, 0.0f)));

  float4 verticalBlend = 0.5f * (
    sceneTexture.Sample(linearClampSampler, input.uv + float2(0.0f, -texelSize.y)) +
    sceneTexture.Sample(linearClampSampler, input.uv + float2(0.0f, texelSize.y)));

  float horizontalWeight = saturate(edge.x);
  float verticalWeight = saturate(edge.y);
  float blendWeight = saturate(max(horizontalWeight, verticalWeight) * parameters.y);

  float4 smoothed = lerp(baseColor, horizontalBlend, horizontalWeight);
  smoothed = lerp(smoothed, verticalBlend, verticalWeight);
  return lerp(baseColor, smoothed, blendWeight);
}
