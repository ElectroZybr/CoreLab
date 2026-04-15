#pragma once

#include <SFML/System/Vector2.hpp>

#include "view/rails/RailPath.h"

namespace view::rails {
enum class RailDirection {
    Left,
    Right,
    Up,
    Down,
};

struct RailPort {
    sf::Vector2f position{0.0f, 0.0f};
    RailDirection direction = RailDirection::Right;
};

class RailBuilder {
  public:
    [[nodiscard]] static RailPath
    straight(sf::Vector2f startPoint, sf::Vector2f endPoint, RailStyle style = {});

    [[nodiscard]] static RailPath arc(sf::Vector2f center,
                                      float radius,
                                      float startAngleRadians,
                                      float endAngleRadians,
                                      RailStyle style = {});

    [[nodiscard]] static RailPath
    orthogonal(const RailPort& startPort, const RailPort& endPort, float turnRadius, RailStyle style = {});
};
} // namespace view::rails
