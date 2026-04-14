#include "../include/RayTracer.hpp"

#include "../utils/Math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace
{
struct Float3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Ray
{
  Float3 origin = {};
  Float3 direction = {};
};

struct Triangle
{
  Float3 v0 = {};
  Float3 v1 = {};
  Float3 v2 = {};
  Float3 normal = {};
  Float3 albedo = {};
  float reflectivity = 0.0f;
};

inline Float3 ToFloat3(const math::Float3& value)
{
  return Float3{value.x, value.y, value.z};
}

inline math::Float3 ToMathFloat3(const Float3& value)
{
  return math::MakeFloat3(value.x, value.y, value.z);
}

inline Float3 Add(const Float3& a, const Float3& b)
{
  return Float3{a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Float3 Sub(const Float3& a, const Float3& b)
{
  return Float3{a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Float3 Mul(const Float3& value, float scalar)
{
  return Float3{value.x * scalar, value.y * scalar, value.z * scalar};
}

inline Float3 Hadamard(const Float3& a, const Float3& b)
{
  return Float3{a.x * b.x, a.y * b.y, a.z * b.z};
}

inline float Dot(const Float3& a, const Float3& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Float3 Cross(const Float3& a, const Float3& b)
{
  return Float3{
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x};
}

inline float Length(const Float3& value)
{
  return std::sqrt(Dot(value, value));
}

inline Float3 Normalize(const Float3& value, const Float3& fallback = Float3{0.0f, 0.0f, 1.0f})
{
  const float len = Length(value);
  if (len < 1.0e-6f)
  {
    return fallback;
  }
  return Mul(value, 1.0f / len);
}

inline Float3 Clamp01(const Float3& value)
{
  return Float3{
    std::clamp(value.x, 0.0f, 1.0f),
    std::clamp(value.y, 0.0f, 1.0f),
    std::clamp(value.z, 0.0f, 1.0f)};
}

inline Float3 Reflect(const Float3& incident, const Float3& normal)
{
  const float d = Dot(incident, normal);
  return Sub(incident, Mul(normal, 2.0f * d));
}

inline Float3 Lerp(const Float3& a, const Float3& b, float t)
{
  return Add(Mul(a, 1.0f - t), Mul(b, t));
}

inline std::uint32_t PackRgba8(const Float3& rgb)
{
  const Float3 clamped = Clamp01(rgb);
  const auto toU8 = [](float v) -> std::uint32_t
  {
    return static_cast<std::uint32_t>(std::lround(v * 255.0f));
  };

  const std::uint32_t r = toU8(clamped.x);
  const std::uint32_t g = toU8(clamped.y);
  const std::uint32_t b = toU8(clamped.z);
  const std::uint32_t a = 255u;
  return (a << 24u) | (b << 16u) | (g << 8u) | r;
}

inline Float3 GammaEncodeApprox(const Float3& linearRgb)
{
  const Float3 clamped = Clamp01(linearRgb);
  return Float3{
    std::pow(clamped.x, 1.0f / 2.2f),
    std::pow(clamped.y, 1.0f / 2.2f),
    std::pow(clamped.z, 1.0f / 2.2f)};
}

std::array<Triangle, 12> MakeUnitCubeTriangles()
{
  const Float3 nNegZ{0.0f, 0.0f, -1.0f};
  const Float3 nPosZ{0.0f, 0.0f, 1.0f};
  const Float3 nNegX{-1.0f, 0.0f, 0.0f};
  const Float3 nPosX{1.0f, 0.0f, 0.0f};
  const Float3 nPosY{0.0f, 1.0f, 0.0f};
  const Float3 nNegY{0.0f, -1.0f, 0.0f};

  const float h = 0.5f;
  const Float3 p000{-h, -h, -h};
  const Float3 p001{-h, -h, h};
  const Float3 p010{-h, h, -h};
  const Float3 p011{-h, h, h};
  const Float3 p100{h, -h, -h};
  const Float3 p101{h, -h, h};
  const Float3 p110{h, h, -h};
  const Float3 p111{h, h, h};

  return std::array<Triangle, 12>{
    Triangle{p000, p010, p110, nNegZ},
    Triangle{p000, p110, p100, nNegZ},
    Triangle{p001, p101, p111, nPosZ},
    Triangle{p001, p111, p011, nPosZ},
    Triangle{p001, p011, p010, nNegX},
    Triangle{p001, p010, p000, nNegX},
    Triangle{p100, p110, p111, nPosX},
    Triangle{p100, p111, p101, nPosX},
    Triangle{p010, p011, p111, nPosY},
    Triangle{p010, p111, p110, nPosY},
    Triangle{p001, p000, p100, nNegY},
    Triangle{p001, p100, p101, nNegY},
  };
}

Float3 TransformCoord(const Float3& position, const math::Matrix& matrix)
{
  const math::Vector v = math::Set(position.x, position.y, position.z, 1.0f);
  const math::Vector transformed = DirectX::XMVector3TransformCoord(v, matrix);
  return ToFloat3(math::StoreFloat3(transformed));
}

Float3 TransformNormal(const Float3& normal, const math::Matrix& matrix)
{
  const math::Vector v = math::Set(normal.x, normal.y, normal.z, 0.0f);
  const math::Vector transformed = DirectX::XMVector3TransformNormal(v, matrix);
  return Normalize(ToFloat3(math::StoreFloat3(transformed)), normal);
}

void AppendCubeTriangles(const CubeState& cube,
                         const math::Float4& cubeColor,
                         float reflectivity,
                         std::vector<Triangle>& triangles)
{
  static const std::array<Triangle, 12> unitTriangles = MakeUnitCubeTriangles();

  const math::Matrix worldMatrix = math::Scaling(cube.extents.x * 2.0f, cube.extents.y * 2.0f, cube.extents.z * 2.0f) *
                                   math::RotationRollPitchYaw(cube.rotation.x, cube.rotation.y, cube.rotation.z) *
                                   math::Translation(cube.position.x, cube.position.y, cube.position.z);

  const Float3 albedo{cubeColor.x, cubeColor.y, cubeColor.z};

  triangles.reserve(triangles.size() + unitTriangles.size());
  for (const Triangle& tri : unitTriangles)
  {
    Triangle out = tri;
    out.v0 = TransformCoord(tri.v0, worldMatrix);
    out.v1 = TransformCoord(tri.v1, worldMatrix);
    out.v2 = TransformCoord(tri.v2, worldMatrix);
    out.normal = TransformNormal(tri.normal, worldMatrix);
    out.albedo = albedo;
    out.reflectivity = reflectivity;
    triangles.push_back(out);
  }
}

bool IntersectTriangle(const Ray& ray, const Triangle& triangle, float& outT, Float3& outNormal, Float3& outAlbedo, float& outReflectivity)
{
  const Float3 e1 = Sub(triangle.v1, triangle.v0);
  const Float3 e2 = Sub(triangle.v2, triangle.v0);
  const Float3 pvec = Cross(ray.direction, e2);
  const float det = Dot(e1, pvec);
  if (std::fabs(det) < 1.0e-8f)
  {
    return false;
  }

  const float invDet = 1.0f / det;
  const Float3 tvec = Sub(ray.origin, triangle.v0);
  const float u = Dot(tvec, pvec) * invDet;
  if (u < 0.0f || u > 1.0f)
  {
    return false;
  }

  const Float3 qvec = Cross(tvec, e1);
  const float v = Dot(ray.direction, qvec) * invDet;
  if (v < 0.0f || u + v > 1.0f)
  {
    return false;
  }

  const float t = Dot(e2, qvec) * invDet;
  if (t <= 1.0e-4f)
  {
    return false;
  }

  outT = t;
  outNormal = triangle.normal;
  outAlbedo = triangle.albedo;
  outReflectivity = triangle.reflectivity;
  return true;
}

bool TraceClosestHit(const Ray& ray, const std::vector<Triangle>& triangles, float& outT, Float3& outNormal, Float3& outAlbedo, float& outReflectivity)
{
  bool hit = false;
  float closestT = std::numeric_limits<float>::max();
  Float3 normal = {};
  Float3 albedo = {};
  float reflectivity = 0.0f;

  for (const Triangle& triangle : triangles)
  {
    float t = 0.0f;
    Float3 triNormal;
    Float3 triAlbedo;
    float triReflectivity = 0.0f;
    if (IntersectTriangle(ray, triangle, t, triNormal, triAlbedo, triReflectivity) && t < closestT)
    {
      hit = true;
      closestT = t;
      normal = triNormal;
      albedo = triAlbedo;
      reflectivity = triReflectivity;
    }
  }

  if (hit)
  {
    outT = closestT;
    outNormal = Normalize(normal, normal);
    outAlbedo = albedo;
    outReflectivity = reflectivity;
  }

  return hit;
}

Float3 ShadeBackground(const Float3& direction)
{
  const float t = 0.5f * (direction.y + 1.0f);
  const Float3 skyA{0.04f, 0.05f, 0.07f};
  const Float3 skyB{0.18f, 0.22f, 0.28f};
  return Lerp(skyA, skyB, std::clamp(t, 0.0f, 1.0f));
}

Float3 ShadePlane(const Ray& ray, float t, const RayTracerSettings& settings, const std::vector<Triangle>& triangles, const Float3& lightDir, const Float3& cameraPos)
{
  const Float3 hitPos = Add(ray.origin, Mul(ray.direction, t));
  const Float3 normal{0.0f, 1.0f, 0.0f};

  const int checkX = static_cast<int>(std::floor(hitPos.x));
  const int checkZ = static_cast<int>(std::floor(hitPos.z));
  const bool odd = ((checkX + checkZ) & 1) != 0;
  const Float3 albedo = odd ? Float3{0.75f, 0.75f, 0.75f} : Float3{0.22f, 0.22f, 0.22f};

  bool shadowed = false;
  Ray shadowRay;
  shadowRay.origin = Add(hitPos, Mul(normal, settings.shadowEpsilon));
  shadowRay.direction = lightDir;
  float shadowT = 0.0f;
  Float3 shadowNormal;
  Float3 shadowAlbedo;
  float shadowReflectivity = 0.0f;
  if (TraceClosestHit(shadowRay, triangles, shadowT, shadowNormal, shadowAlbedo, shadowReflectivity))
  {
    shadowed = true;
  }

  const float diffuse = std::max(0.0f, Dot(normal, lightDir));
  float lighting = 0.25f + diffuse * 0.75f;
  if (shadowed)
  {
    lighting *= 0.35f;
  }

  const Float3 base = Mul(albedo, lighting);
  const Float3 viewDir = Normalize(Sub(cameraPos, hitPos));
  const Float3 halfVec = Normalize(Add(lightDir, viewDir));
  const float spec = std::pow(std::max(0.0f, Dot(normal, halfVec)), settings.specularPower) * settings.specularStrength;
  return Add(base, Float3{spec, spec, spec});
}

Float3 TraceRay(const Ray& ray,
                const RayTracerSettings& settings,
                const std::vector<Triangle>& triangles,
                const Float3& lightDir,
                const Float3& cameraPos,
                std::uint32_t bounce)
{
  float t = 0.0f;
  Float3 normal;
  Float3 albedo;
  float reflectivity = 0.0f;

  bool hitObject = TraceClosestHit(ray, triangles, t, normal, albedo, reflectivity);

  float planeT = 0.0f;
  bool hitPlane = false;
  if (std::fabs(ray.direction.y) > 1.0e-6f)
  {
    planeT = (-1.0f - ray.origin.y) / ray.direction.y;
    hitPlane = planeT > 1.0e-4f;
  }

  if (hitPlane && (!hitObject || planeT < t))
  {
    return ShadePlane(ray, planeT, settings, triangles, lightDir, cameraPos);
  }

  if (!hitObject)
  {
    return ShadeBackground(ray.direction);
  }

  const Float3 hitPos = Add(ray.origin, Mul(ray.direction, t));

  bool shadowed = false;
  Ray shadowRay;
  shadowRay.origin = Add(hitPos, Mul(normal, settings.shadowEpsilon));
  shadowRay.direction = lightDir;
  float shadowT = 0.0f;
  Float3 shadowNormal;
  Float3 shadowAlbedo;
  float shadowReflectivity = 0.0f;
  if (TraceClosestHit(shadowRay, triangles, shadowT, shadowNormal, shadowAlbedo, shadowReflectivity))
  {
    shadowed = true;
  }

  const float diffuse = std::max(0.0f, Dot(normal, lightDir));
  float lighting = 0.25f + diffuse * 0.75f;
  if (shadowed)
  {
    lighting *= 0.2f;
  }

  Float3 color = Mul(albedo, lighting);

  const Float3 viewDir = Normalize(Sub(cameraPos, hitPos));
  const Float3 halfVec = Normalize(Add(lightDir, viewDir));
  const float spec = std::pow(std::max(0.0f, Dot(normal, halfVec)), settings.specularPower) * settings.specularStrength;
  color = Add(color, Float3{spec, spec, spec});

  if (bounce < settings.maxBounces && reflectivity > 0.0f && settings.reflectionStrength > 0.0f)
  {
    Ray reflectRay;
    reflectRay.origin = Add(hitPos, Mul(normal, settings.shadowEpsilon));
    reflectRay.direction = Normalize(Reflect(ray.direction, normal));
    const Float3 reflected = TraceRay(reflectRay, settings, triangles, lightDir, cameraPos, bounce + 1);
    color = Lerp(color, reflected, settings.reflectionStrength * reflectivity);
  }

  return color;
}
}  // namespace

RayTracer::RayTracer() = default;

void RayTracer::SetSettings(const RayTracerSettings& settings)
{
  settings_ = settings;
}

const RayTracerSettings& RayTracer::GetSettings() const
{
  return settings_;
}

void RayTracer::Render(const Camera& camera,
                       const CubeState& cubeA,
                       const math::Float4& cubeAColor,
                       const CubeState& cubeB,
                       const math::Float4& cubeBColor,
                       std::uint32_t width,
                       std::uint32_t height,
                       std::vector<std::uint32_t>& outRgba8)
{
  outRgba8.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

  std::vector<Triangle> triangles;
  triangles.reserve(64);

  AppendCubeTriangles(cubeA, cubeAColor, 1.0f, triangles);
  AppendCubeTriangles(cubeB, cubeBColor, 0.0f, triangles);

  const Float3 cameraPos = ToFloat3(camera.GetPosition());
  const float yaw = camera.GetYaw();
  const float pitch = camera.GetPitch();

  const float cosPitch = std::cos(pitch);
  const Float3 forward = Normalize(Float3{std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch});
  const Float3 worldUp{0.0f, 1.0f, 0.0f};
  const Float3 right = Normalize(Cross(worldUp, forward), Float3{1.0f, 0.0f, 0.0f});
  const Float3 up = Normalize(Cross(forward, right), worldUp);

  const float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1u));
  const float fovRadians = settings_.fieldOfViewDegrees * (math::kPi / 180.0f);
  const float tanHalfFov = std::tan(fovRadians * 0.5f);

  const Float3 lightDir = Normalize(Float3{0.5f, 1.0f, -0.4f});

  for (std::uint32_t y = 0; y < height; ++y)
  {
    for (std::uint32_t x = 0; x < width; ++x)
    {
      const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
      const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);

      const float ndcX = (u * 2.0f - 1.0f) * aspect * tanHalfFov;
      const float ndcY = (1.0f - v * 2.0f) * tanHalfFov;

      Ray ray;
      ray.origin = cameraPos;
      ray.direction = Normalize(Add(forward, Add(Mul(right, ndcX), Mul(up, ndcY))));

      const Float3 linearColor = TraceRay(ray, settings_, triangles, lightDir, cameraPos, 0);
      const Float3 gammaColor = GammaEncodeApprox(linearColor);
      outRgba8[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + x] = PackRgba8(gammaColor);
    }
  }
}
