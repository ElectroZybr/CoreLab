#include "view/rails/ArcRailSegment.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace view::rails {
ArcRailSegment::ArcRailSegment(sf::Vector2f center,
                               float radius,
                               float startAngleRadians,
                               float endAngleRadians,
                               float thickness,
                               sf::Color color)
    : RailSegment(thickness, color), m_center(center), m_radius(std::max(radius, 0.0f)),
      m_startAngleRadians(startAngleRadians), m_endAngleRadians(endAngleRadians) {
}

void ArcRailSegment::setArc(sf::Vector2f center,
                            float radius,
                            float startAngleRadians,
                            float endAngleRadians) {
    m_center = center;
    m_radius = std::max(radius, 0.0f);
    m_startAngleRadians = startAngleRadians;
    m_endAngleRadians = endAngleRadians;
    markDirty();
}

float ArcRailSegment::getLength() const {
    return std::abs(m_endAngleRadians - m_startAngleRadians) * m_radius;
}

sf::Vector2f ArcRailSegment::getStartPoint() const {
    return {m_center.x + std::cos(m_startAngleRadians) * m_radius,
            m_center.y + std::sin(m_startAngleRadians) * m_radius};
}

sf::Vector2f ArcRailSegment::getEndPoint() const {
    return {m_center.x + std::cos(m_endAngleRadians) * m_radius,
            m_center.y + std::sin(m_endAngleRadians) * m_radius};
}

sf::Vector2f ArcRailSegment::samplePoint(float distance) const {
    if (m_radius <= 0.0f) {
        return m_center;
    }

    const float length = getLength();
    const float t = length > 0.0f ? clampDistance(distance) / length : 0.0f;
    const float angle = m_startAngleRadians + (m_endAngleRadians - m_startAngleRadians) * t;
    return {m_center.x + std::cos(angle) * m_radius, m_center.y + std::sin(angle) * m_radius};
}

sf::Vector2f ArcRailSegment::sampleTangent(float distance) const {
    if (m_radius <= 0.0f) {
        return {0.0f, 0.0f};
    }

    const float length = getLength();
    const float t = length > 0.0f ? clampDistance(distance) / length : 0.0f;
    const float angle = m_startAngleRadians + (m_endAngleRadians - m_startAngleRadians) * t;
    const float direction = m_endAngleRadians >= m_startAngleRadians ? 1.0f : -1.0f;
    return normalizeOrZero({-std::sin(angle) * direction, std::cos(angle) * direction});
}

void ArcRailSegment::rebuildGeometry() const {
    if (m_radius <= 0.0f) {
        m_geometry = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
        return;
    }

    const float angleSpan = std::abs(m_endAngleRadians - m_startAngleRadians);
    const std::size_t segmentCount =
        std::max<std::size_t>(8, static_cast<std::size_t>(std::ceil(angleSpan * m_radius / 24.0f)));
    std::vector<sf::Vector2f> points;
    points.reserve(segmentCount + 1);

    for (std::size_t index = 0; index <= segmentCount; ++index) {
        const float t = static_cast<float>(index) / static_cast<float>(segmentCount);
        const float angle = m_startAngleRadians + (m_endAngleRadians - m_startAngleRadians) * t;
        points.push_back({m_center.x + std::cos(angle) * m_radius, m_center.y + std::sin(angle) * m_radius});
    }

    m_geometry = buildThickPolyline(points, getThickness(), getColor());
}
} // namespace view::rails
