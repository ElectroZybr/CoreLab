#include "view/rails/StraightRailSegment.h"

#include <array>
#include <cmath>

namespace view::rails {
StraightRailSegment::StraightRailSegment(sf::Vector2f startPoint,
                                         sf::Vector2f endPoint,
                                         float thickness,
                                         sf::Color color)
    : RailSegment(thickness, color), m_startPoint(startPoint), m_endPoint(endPoint) {
}

void StraightRailSegment::setEndpoints(sf::Vector2f startPoint, sf::Vector2f endPoint) {
    m_startPoint = startPoint;
    m_endPoint = endPoint;
    markDirty();
}

float StraightRailSegment::getLength() const {
    const sf::Vector2f delta = m_endPoint - m_startPoint;
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

sf::Vector2f StraightRailSegment::samplePoint(float distance) const {
    const float length = getLength();
    if (length <= 0.0f) {
        return m_startPoint;
    }

    const float t = clampDistance(distance) / length;
    return m_startPoint + (m_endPoint - m_startPoint) * t;
}

sf::Vector2f StraightRailSegment::sampleTangent(float) const {
    return normalizeOrZero(m_endPoint - m_startPoint);
}

void StraightRailSegment::rebuildGeometry() const {
    const std::array<sf::Vector2f, 2> points{m_startPoint, m_endPoint};
    m_geometry = buildThickPolyline(points, getThickness(), getColor());
}
} // namespace view::rails
