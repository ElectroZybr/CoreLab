#include "view/rails/RailBuilder.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace view::rails {
namespace {
sf::Vector2f directionVector(RailDirection direction) {
    switch (direction) {
    case RailDirection::Left:
        return {-1.0f, 0.0f};
    case RailDirection::Right:
        return {1.0f, 0.0f};
    case RailDirection::Up:
        return {0.0f, -1.0f};
    case RailDirection::Down:
        return {0.0f, 1.0f};
    }

    return {0.0f, 0.0f};
}

float angleForDirection(RailDirection direction) {
    switch (direction) {
    case RailDirection::Right:
        return 0.0f;
    case RailDirection::Down:
        return std::numbers::pi_v<float> * 0.5f;
    case RailDirection::Left:
        return std::numbers::pi_v<float>;
    case RailDirection::Up:
        return -std::numbers::pi_v<float> * 0.5f;
    }

    return 0.0f;
}
} // namespace

RailPath RailBuilder::straight(sf::Vector2f startPoint, sf::Vector2f endPoint, RailStyle style) {
    RailPath path(style);
    path.appendStraight(startPoint, endPoint);
    return path;
}

RailPath RailBuilder::arc(
    sf::Vector2f center, float radius, float startAngleRadians, float endAngleRadians, RailStyle style) {
    RailPath path(style);
    path.appendArc(center, radius, startAngleRadians, endAngleRadians);
    return path;
}

RailPath RailBuilder::orthogonal(const RailPort& startPort,
                                 const RailPort& endPort,
                                 float turnRadius,
                                 RailStyle style) {
    if (turnRadius <= 0.0f) {
        throw std::invalid_argument("RailBuilder::orthogonal requires positive turn radius");
    }

    const sf::Vector2f startDirection = directionVector(startPort.direction);
    const sf::Vector2f endDirection = directionVector(endPort.direction);
    const float dot = startDirection.x * endDirection.x + startDirection.y * endDirection.y;

    if (std::abs(dot) > 0.001f) {
        throw std::invalid_argument("RailBuilder::orthogonal requires orthogonal start/end directions");
    }

    const sf::Vector2f startTurnPoint = startPort.position + startDirection * turnRadius;
    const sf::Vector2f endTurnPoint = endPort.position + endDirection * turnRadius;
    const sf::Vector2f center{startTurnPoint.x + endDirection.x * turnRadius,
                              startTurnPoint.y + endDirection.y * turnRadius};

    RailPath path(style);
    if (std::abs(startTurnPoint.x - startPort.position.x) > 0.001f ||
        std::abs(startTurnPoint.y - startPort.position.y) > 0.001f) {
        path.appendStraight(startPort.position, startTurnPoint);
    }

    path.appendArc(center,
                   turnRadius,
                   angleForDirection(startPort.direction) + std::numbers::pi_v<float>,
                   angleForDirection(endPort.direction) + std::numbers::pi_v<float>);

    if (std::abs(endPort.position.x - endTurnPoint.x) > 0.001f ||
        std::abs(endPort.position.y - endTurnPoint.y) > 0.001f) {
        path.appendStraight(endTurnPoint, endPort.position);
    }

    return path;
}
} // namespace view::rails
