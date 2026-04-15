#pragma once

#include "view/rails/RailSegment.h"

namespace view::rails {
class BezierRailSegment final : public RailSegment {
  public:
    BezierRailSegment(sf::Vector2f p0 = {0.0f, 0.0f},
                      sf::Vector2f p1 = {0.0f, 0.0f},
                      sf::Vector2f p2 = {0.0f, 0.0f},
                      sf::Vector2f p3 = {0.0f, 0.0f},
                      float thickness = 6.0f,
                      sf::Color color = sf::Color(116, 134, 165));

    void setControlPoints(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3);

    [[nodiscard]] float getLength() const override;
    [[nodiscard]] sf::Vector2f getStartPoint() const override {
        return m_p0;
    }
    [[nodiscard]] sf::Vector2f getEndPoint() const override {
        return m_p3;
    }
    [[nodiscard]] sf::Vector2f samplePoint(float distance) const override;
    [[nodiscard]] sf::Vector2f sampleTangent(float distance) const override;

  private:
    [[nodiscard]] static sf::Vector2f
    cubicBezier(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t);
    [[nodiscard]] static sf::Vector2f
    cubicBezierDerivative(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t);

    void rebuildGeometry() const override;

    sf::Vector2f m_p0{0.0f, 0.0f};
    sf::Vector2f m_p1{0.0f, 0.0f};
    sf::Vector2f m_p2{0.0f, 0.0f};
    sf::Vector2f m_p3{0.0f, 0.0f};
};
} // namespace view::rails
