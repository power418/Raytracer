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
  float3 rgbM = sceneTexture.Sample(linearClampSampler, input.uv).rgb;
  float3 rgbNW = sceneTexture.Sample(linearClampSampler, input.uv + texelSize * float2(-1.0f, -1.0f)).rgb;
  float3 rgbNE = sceneTexture.Sample(linearClampSampler, input.uv + texelSize * float2(1.0f, -1.0f)).rgb;
  float3 rgbSW = sceneTexture.Sample(linearClampSampler, input.uv + texelSize * float2(-1.0f, 1.0f)).rgb;
  float3 rgbSE = sceneTexture.Sample(linearClampSampler, input.uv + texelSize * float2(1.0f, 1.0f)).rgb;

  float lumaM = Luma(rgbM);
  float lumaNW = Luma(rgbNW);
  float lumaNE = Luma(rgbNE);
  float lumaSW = Luma(rgbSW);
  float lumaSE = Luma(rgbSE);

  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
  float edgeContrast = lumaMax - lumaMin;
  if (edgeContrast < parameters.x)
  {
    return float4(rgbM, 1.0f);
  }

  float2 dir;
  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
  dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125f, 0.0078125f);
  float inverseDirAdjustment = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
  dir = clamp(dir * inverseDirAdjustment, float2(-8.0f, -8.0f), float2(8.0f, 8.0f)) * texelSize;

  float3 rgbA = 0.5f * (
    sceneTexture.Sample(linearClampSampler, input.uv + dir * (1.0f / 3.0f - 0.5f)).rgb +
    sceneTexture.Sample(linearClampSampler, input.uv + dir * (2.0f / 3.0f - 0.5f)).rgb);

  float3 rgbB = rgbA * 0.5f + 0.25f * (
    sceneTexture.Sample(linearClampSampler, input.uv + dir * -0.5f).rgb +
    sceneTexture.Sample(linearClampSampler, input.uv + dir * 0.5f).rgb);

  float lumaB = Luma(rgbB);
  if (lumaB < lumaMin || lumaB > lumaMax)
  {
    return float4(rgbA, 1.0f);
  }

  return float4(lerp(rgbM, rgbB, parameters.y), 1.0f);
}
