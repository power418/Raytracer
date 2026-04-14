cbuffer RayTraceConstants : register(b0)
{
  float4 gCameraPosition;
  float4 gCameraForward;
  float4 gCameraRight;
  float4 gCameraUp;
  float4 gLightDirection;
  float4 gScreenInfo;
  float4 gObstacleColor;
  float4 gObstaclePosition;
  float4 gObstacleRotation;
  float4 gObstacleExtents;
  float4 gPlayerColor;
  float4 gPlayerPosition;
  float4 gPlayerRotation;
  float4 gPlayerExtents;
  float4 gRayTraceSettings;
}

RWTexture2D<float4> gOutput : register(u0);

struct Ray
{
  float3 origin;
  float3 direction;
};

struct SceneObject
{
  float3 position;
  float3 rotation;
  float3 extents;
  float3 color;
  float reflectivity;
};

struct HitInfo
{
  bool hit;
  float distance;
  float3 position;
  float3 normal;
  float3 color;
  float reflectivity;
};

static const float kPi = 3.141592654f;

float3 RotateX(float3 value, float angle)
{
  float s = sin(angle);
  float c = cos(angle);
  return float3(value.x, value.y * c - value.z * s, value.y * s + value.z * c);
}

float3 RotateY(float3 value, float angle)
{
  float s = sin(angle);
  float c = cos(angle);
  return float3(value.x * c + value.z * s, value.y, -value.x * s + value.z * c);
}

float3 RotateZ(float3 value, float angle)
{
  float s = sin(angle);
  float c = cos(angle);
  return float3(value.x * c - value.y * s, value.x * s + value.y * c, value.z);
}

float3 RotatePitchYawRoll(float3 value, float3 rotation)
{
  return RotateZ(RotateY(RotateX(value, rotation.x), rotation.y), rotation.z);
}

float3 InverseRotatePitchYawRoll(float3 value, float3 rotation)
{
  return RotateX(RotateY(RotateZ(value, -rotation.z), -rotation.y), -rotation.x);
}

float3 SafeNormalize(float3 value, float3 fallback)
{
  float lengthSquared = dot(value, value);
  if (lengthSquared < 1.0e-8f)
  {
    return fallback;
  }

  return value * rsqrt(lengthSquared);
}

float SafeReciprocal(float value)
{
  return abs(value) > 1.0e-6f ? rcp(value) : (value >= 0.0f ? 1.0e6f : -1.0e6f);
}

float3 BackgroundColor(float3 direction)
{
  float t = saturate(0.5f * (direction.y + 1.0f));
  float3 skyA = float3(0.04f, 0.05f, 0.07f);
  float3 skyB = float3(0.18f, 0.22f, 0.28f);
  return lerp(skyA, skyB, t);
}

bool IntersectPlane(Ray ray, out HitInfo hit)
{
  hit.hit = false;
  if (abs(ray.direction.y) <= 1.0e-6f)
  {
    return false;
  }

  float t = (-1.0f - ray.origin.y) / ray.direction.y;
  if (t <= 1.0e-4f)
  {
    return false;
  }

  float3 position = ray.origin + ray.direction * t;
  int checkX = (int)floor(position.x);
  int checkZ = (int)floor(position.z);
  bool odd = ((checkX + checkZ) & 1) != 0;

  hit.hit = true;
  hit.distance = t;
  hit.position = position;
  hit.normal = float3(0.0f, 1.0f, 0.0f);
  hit.color = odd ? float3(0.75f, 0.75f, 0.75f) : float3(0.22f, 0.22f, 0.22f);
  hit.reflectivity = 0.05f;
  return true;
}

bool IntersectOrientedBox(Ray ray, SceneObject objectData, out HitInfo hit)
{
  hit.hit = false;

  float3 localOrigin = InverseRotatePitchYawRoll(ray.origin - objectData.position, objectData.rotation);
  float3 localDirection = InverseRotatePitchYawRoll(ray.direction, objectData.rotation);

  float3 invDirection = float3(SafeReciprocal(localDirection.x),
                               SafeReciprocal(localDirection.y),
                               SafeReciprocal(localDirection.z));
  float3 minCorner = -objectData.extents;
  float3 maxCorner = objectData.extents;

  float3 t0 = (minCorner - localOrigin) * invDirection;
  float3 t1 = (maxCorner - localOrigin) * invDirection;
  float3 tMin3 = min(t0, t1);
  float3 tMax3 = max(t0, t1);

  float tNear = max(max(tMin3.x, tMin3.y), tMin3.z);
  float tFar = min(min(tMax3.x, tMax3.y), tMax3.z);
  if (tNear > tFar || tFar <= 1.0e-4f)
  {
    return false;
  }

  float tHit = tNear > 1.0e-4f ? tNear : tFar;
  float3 localHitPosition = localOrigin + localDirection * tHit;
  float3 normalizedHit = abs(localHitPosition / max(objectData.extents, float3(1.0e-4f, 1.0e-4f, 1.0e-4f)));

  float3 localNormal = float3(0.0f, 0.0f, 0.0f);
  if (normalizedHit.x >= normalizedHit.y && normalizedHit.x >= normalizedHit.z)
  {
    localNormal.x = localHitPosition.x >= 0.0f ? 1.0f : -1.0f;
  }
  else if (normalizedHit.y >= normalizedHit.z)
  {
    localNormal.y = localHitPosition.y >= 0.0f ? 1.0f : -1.0f;
  }
  else
  {
    localNormal.z = localHitPosition.z >= 0.0f ? 1.0f : -1.0f;
  }

  hit.hit = true;
  hit.distance = tHit;
  hit.position = ray.origin + ray.direction * tHit;
  hit.normal = SafeNormalize(RotatePitchYawRoll(localNormal, objectData.rotation), float3(0.0f, 1.0f, 0.0f));
  hit.color = objectData.color;
  hit.reflectivity = objectData.reflectivity;
  return true;
}

bool TraceScene(Ray ray, SceneObject obstacleObject, SceneObject playerObject, out HitInfo bestHit)
{
  bestHit.hit = false;
  bestHit.distance = 1.0e20f;

  HitInfo candidate = (HitInfo)0;

  if (IntersectOrientedBox(ray, obstacleObject, candidate) && candidate.distance < bestHit.distance)
  {
    bestHit = candidate;
  }

  if (IntersectOrientedBox(ray, playerObject, candidate) && candidate.distance < bestHit.distance)
  {
    bestHit = candidate;
  }

  if (IntersectPlane(ray, candidate) && candidate.distance < bestHit.distance)
  {
    bestHit = candidate;
  }

  return bestHit.hit;
}

bool TraceShadow(float3 origin, float3 normal, float3 lightDirection, SceneObject obstacleObject, SceneObject playerObject, float shadowBias)
{
  Ray shadowRay;
  shadowRay.origin = origin + normal * shadowBias;
  shadowRay.direction = lightDirection;

  HitInfo shadowHit = (HitInfo)0;
  if (!TraceScene(shadowRay, obstacleObject, playerObject, shadowHit))
  {
    return false;
  }

  return shadowHit.distance > shadowBias;
}

float3 ShadeHit(Ray ray,
                HitInfo hit,
                float3 cameraPosition,
                float3 lightDirection,
                SceneObject obstacleObject,
                SceneObject playerObject,
                float shadowBias,
                float specularPower,
                float specularStrength)
{
  bool shadowed = TraceShadow(hit.position, hit.normal, lightDirection, obstacleObject, playerObject, shadowBias);
  float diffuse = max(0.0f, dot(hit.normal, lightDirection));
  float lighting = 0.22f + diffuse * 0.78f;
  if (shadowed)
  {
    lighting *= 0.3f;
  }

  float3 color = hit.color * lighting;
  float3 viewDirection = SafeNormalize(cameraPosition - hit.position, float3(0.0f, 0.0f, 1.0f));
  float3 halfVector = SafeNormalize(lightDirection + viewDirection, hit.normal);
  float specular = pow(max(0.0f, dot(hit.normal, halfVector)), specularPower) * specularStrength;
  return color + specular.xxx;
}

float3 TracePrimaryRay(Ray ray,
                       float3 cameraPosition,
                       float3 lightDirection,
                       SceneObject obstacleObject,
                       SceneObject playerObject,
                       float shadowBias,
                       float specularPower,
                       float specularStrength,
                       float reflectionStrength)
{
  HitInfo hit = (HitInfo)0;
  if (!TraceScene(ray, obstacleObject, playerObject, hit))
  {
    return BackgroundColor(ray.direction);
  }

  float3 color = ShadeHit(ray,
                          hit,
                          cameraPosition,
                          lightDirection,
                          obstacleObject,
                          playerObject,
                          shadowBias,
                          specularPower,
                          specularStrength);

  if (reflectionStrength > 0.0f && hit.reflectivity > 0.0f)
  {
    Ray reflectionRay;
    reflectionRay.origin = hit.position + hit.normal * shadowBias;
    reflectionRay.direction = SafeNormalize(reflect(ray.direction, hit.normal), hit.normal);

    HitInfo reflectionHit = (HitInfo)0;
    float3 reflectedColor = TraceScene(reflectionRay, obstacleObject, playerObject, reflectionHit)
                                ? ShadeHit(reflectionRay,
                                           reflectionHit,
                                           cameraPosition,
                                           lightDirection,
                                           obstacleObject,
                                           playerObject,
                                           shadowBias,
                                           specularPower,
                                           specularStrength)
                                : BackgroundColor(reflectionRay.direction);

    color = lerp(color, reflectedColor, saturate(reflectionStrength * hit.reflectivity));
  }

  return color;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
  uint2 pixel = dispatchThreadId.xy;
  uint width = (uint)gScreenInfo.x;
  uint height = (uint)gScreenInfo.y;
  if (pixel.x >= width || pixel.y >= height)
  {
    return;
  }

  float2 uv = (float2(pixel) + 0.5f) / float2(width, height);
  float ndcX = (uv.x * 2.0f - 1.0f) * gScreenInfo.w * gScreenInfo.z;
  float ndcY = (1.0f - uv.y * 2.0f) * gScreenInfo.z;

  Ray ray;
  ray.origin = gCameraPosition.xyz;
  ray.direction = SafeNormalize(gCameraForward.xyz + gCameraRight.xyz * ndcX + gCameraUp.xyz * ndcY,
                                gCameraForward.xyz);

  SceneObject obstacleObject;
  obstacleObject.position = gObstaclePosition.xyz;
  obstacleObject.rotation = gObstacleRotation.xyz;
  obstacleObject.extents = gObstacleExtents.xyz;
  obstacleObject.color = gObstacleColor.xyz;
  obstacleObject.reflectivity = 1.0f;

  SceneObject playerObject;
  playerObject.position = gPlayerPosition.xyz;
  playerObject.rotation = gPlayerRotation.xyz;
  playerObject.extents = gPlayerExtents.xyz;
  playerObject.color = gPlayerColor.xyz;
  playerObject.reflectivity = 0.0f;

  float3 lightDirection = SafeNormalize(gLightDirection.xyz, float3(0.0f, 1.0f, 0.0f));
  float3 linearColor = TracePrimaryRay(ray,
                                       gCameraPosition.xyz,
                                       lightDirection,
                                       obstacleObject,
                                       playerObject,
                                       gRayTraceSettings.x,
                                       gRayTraceSettings.y,
                                       gRayTraceSettings.z,
                                       gRayTraceSettings.w);

  float3 gammaColor = pow(saturate(linearColor), 1.0f / 2.2f);
  gOutput[pixel] = float4(gammaColor, 1.0f);
}
