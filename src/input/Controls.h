#pragma once

#include <SFML/Graphics.hpp>

#include "core/Camera.h"

namespace Controls
{
void handleEvents(
    sf::RenderWindow& window,
    Camera& camera,
    sf::View& sceneView,
    sf::Vector2f resetPosition,
    float resetZoom
);

[[nodiscard]] sf::Vector2f readMovement();
[[nodiscard]] sf::Vector2f readRamMovement();
}
