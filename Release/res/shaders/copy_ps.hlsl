Texture2D sceneTexture : register(t0);
SamplerState linearClampSampler : register(s0);

struct PSInput
{
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
  return sceneTexture.Sample(linearClampSampler, input.uv);
}
