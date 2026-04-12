cbuffer GridConstants : register(b0)
{
  matrix worldViewProjection;
  matrix world;
  float4 minorColor;
  float4 majorColor;
  float4 gridParams;
  float4 cameraWorldPosition;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 worldPosition : TEXCOORD0;
};

float GridLineFactor(float2 coord)
{
  float2 grid = abs(frac(coord - 0.5f) - 0.5f) / max(fwidth(coord), 1.0e-5f);
  return 1.0f - saturate(min(grid.x, grid.y));
}

float4 main(PSInput input) : SV_TARGET
{
  float cellSize = max(gridParams.x, 1.0e-4f);
  float majorScale = max(gridParams.y, 1.0f);
  float fadeDistance = max(gridParams.z, 1.0f);

  float2 worldXZ = input.worldPosition.xz;
  float minor = GridLineFactor(worldXZ / cellSize);
  float major = GridLineFactor(worldXZ / (cellSize * majorScale));

  float axisX = 1.0f - saturate(abs(input.worldPosition.x) / max(fwidth(input.worldPosition.x) * 2.0f, 1.0e-5f));
  float axisZ = 1.0f - saturate(abs(input.worldPosition.z) / max(fwidth(input.worldPosition.z) * 2.0f, 1.0e-5f));
  major = max(major, max(axisX, axisZ));

  float distanceToCamera = distance(worldXZ, cameraWorldPosition.xz);
  float fade = saturate(1.0f - distanceToCamera / fadeDistance);

  float3 color = minorColor.rgb * minor;
  color = max(color, majorColor.rgb * major);
  color *= fade;

  return float4(color, 1.0f);
}
