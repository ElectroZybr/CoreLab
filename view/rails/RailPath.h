#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "view/rails/ArcRailSegment.h"
#include "view/rails/StraightRailSegment.h"

namespace view::rails {
struct RailStyle {
    float thickness = 6.0f;
    sf::Color color = sf::Color(116, 134, 165);
};

class RailPath : public sf::Drawable {
  public:
    RailPath() = default;
    explicit RailPath(RailStyle style);

    void setStyle(RailStyle style);
    [[nodiscard]] RailStyle getStyle() const {
        return m_style;
    }

    void appendStraight(sf::Vector2f startPoint, sf::Vector2f endPoint);
    void appendArc(sf::Vector2f center, float radius, float startAngleRadians, float endAngleRadians);
    void appendPath(RailPath path);

    [[nodiscard]] bool isEmpty() const {
        return m_segments.empty();
    }
    [[nodiscard]] float getLength() const;
    [[nodiscard]] sf::Vector2f getStartPoint() const;
    [[nodiscard]] sf::Vector2f getEndPoint() const;
    [[nodiscard]] sf::Vector2f samplePoint(float distance) const;
    [[nodiscard]] sf::Vector2f sampleTangent(float distance) const;

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    RailStyle m_style;
    std::vector<std::unique_ptr<RailSegment>> m_segments;
};
} // namespace view::rails
