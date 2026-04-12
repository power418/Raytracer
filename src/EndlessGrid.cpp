#include "EndlessGrid.hpp"

#include "utils/Math.hpp"

#include <cmath>

namespace
{
float SnapToStep(float value, float step)
{
  if (step <= 1.0e-6f)
  {
    return value;
  }

  return std::floor(value / step) * step;
}
}  // namespace

EndlessGrid::EndlessGrid() = default;

void EndlessGrid::SetSettings(const Settings& settings)
{
  settings_ = settings;
}

const EndlessGrid::Settings& EndlessGrid::GetSettings() const
{
  return settings_;
}

math::Matrix EndlessGrid::BuildWorldMatrix(const math::Float3& cameraPosition) const
{
  const float snapStep = settings_.cellSize * settings_.majorScale;
  const float snappedX = SnapToStep(cameraPosition.x, snapStep);
  const float snappedZ = SnapToStep(cameraPosition.z, snapStep);

  return math::Scaling(settings_.halfExtent, 1.0f, settings_.halfExtent) *
         math::Translation(snappedX, settings_.height, snappedZ);
}
