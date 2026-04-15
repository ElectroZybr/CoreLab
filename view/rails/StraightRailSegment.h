#pragma once

#include "view/rails/RailSegment.h"

namespace view::rails {
class StraightRailSegment final : public RailSegment {
  public:
    StraightRailSegment(sf::Vector2f startPoint = {0.0f, 0.0f},
                        sf::Vector2f endPoint = {0.0f, 0.0f},
                        float thickness = 6.0f,
                        sf::Color color = sf::Color(116, 134, 165));

    void setEndpoints(sf::Vector2f startPoint, sf::Vector2f endPoint);

    [[nodiscard]] float getLength() const override;
    [[nodiscard]] sf::Vector2f getStartPoint() const override {
        return m_startPoint;
    }
    [[nodiscard]] sf::Vector2f getEndPoint() const override {
        return m_endPoint;
    }
    [[nodiscard]] sf::Vector2f samplePoint(float distance) const override;
    [[nodiscard]] sf::Vector2f sampleTangent(float distance) const override;

  private:
    void rebuildGeometry() const override;

    sf::Vector2f m_startPoint{0.0f, 0.0f};
    sf::Vector2f m_endPoint{0.0f, 0.0f};
};
} // namespace view::rails
