#pragma once

#include "../utils/MathTypes.hpp"

class EndlessGrid
{
public:
  struct Settings
  {
    float cellSize = 1.0f;
    float majorScale = 10.0f;
    float halfExtent = 200.0f;
    float height = -1.0f;
    float fadeDistance = 120.0f;
    math::Float4 minorColor = {0.18f, 0.18f, 0.18f, 1.0f};
    math::Float4 majorColor = {0.38f, 0.38f, 0.38f, 1.0f};
  };

  EndlessGrid();

  void SetSettings(const Settings& settings);
  const Settings& GetSettings() const;

  math::Matrix BuildWorldMatrix(const math::Float3& cameraPosition) const;

private:
  Settings settings_ = {};
};
