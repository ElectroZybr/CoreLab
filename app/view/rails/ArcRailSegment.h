#pragma once

#include "view/rails/RailSegment.h"

namespace view::rails {
class ArcRailSegment final : public RailSegment {
  public:
    ArcRailSegment(sf::Vector2f center = {0.0f, 0.0f},
                   float radius = 0.0f,
                   float startAngleRadians = 0.0f,
                   float endAngleRadians = 0.0f,
                   float thickness = 6.0f,
                   sf::Color color = sf::Color(116, 134, 165));

    void setArc(sf::Vector2f center, float radius, float startAngleRadians, float endAngleRadians);

    [[nodiscard]] float getLength() const override;
    [[nodiscard]] sf::Vector2f getStartPoint() const override;
    [[nodiscard]] sf::Vector2f getEndPoint() const override;
    [[nodiscard]] sf::Vector2f samplePoint(float distance) const override;
    [[nodiscard]] sf::Vector2f sampleTangent(float distance) const override;

  private:
    void rebuildGeometry() const override;

    sf::Vector2f m_center{0.0f, 0.0f};
    float m_radius = 0.0f;
    float m_startAngleRadians = 0.0f;
    float m_endAngleRadians = 0.0f;
};
} // namespace view::rails
