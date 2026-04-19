#include "view/rails/BezierRailSegment.h"

#include <array>
#include <cmath>
#include <vector>

namespace {
constexpr std::size_t kBezierSampleCount = 32;
}

namespace view::rails {
BezierRailSegment::BezierRailSegment(
    sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float thickness, sf::Color color)
    : RailSegment(thickness, color), m_p0(p0), m_p1(p1), m_p2(p2), m_p3(p3) {
}

void BezierRailSegment::setControlPoints(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3) {
    m_p0 = p0;
    m_p1 = p1;
    m_p2 = p2;
    m_p3 = p3;
    markDirty();
}

float BezierRailSegment::getLength() const {
    float length = 0.0f;
    sf::Vector2f previousPoint = m_p0;

    for (std::size_t index = 1; index <= kBezierSampleCount; ++index) {
        const float t = static_cast<float>(index) / static_cast<float>(kBezierSampleCount);
        const sf::Vector2f point = cubicBezier(m_p0, m_p1, m_p2, m_p3, t);
        const sf::Vector2f delta = point - previousPoint;
        length += std::sqrt(delta.x * delta.x + delta.y * delta.y);
        previousPoint = point;
    }

    return length;
}

sf::Vector2f BezierRailSegment::samplePoint(float distance) const {
    const float length = getLength();
    const float t = length > 0.0f ? clampDistance(distance) / length : 0.0f;
    return cubicBezier(m_p0, m_p1, m_p2, m_p3, t);
}

sf::Vector2f BezierRailSegment::sampleTangent(float distance) const {
    const float length = getLength();
    const float t = length > 0.0f ? clampDistance(distance) / length : 0.0f;
    return normalizeOrZero(cubicBezierDerivative(m_p0, m_p1, m_p2, m_p3, t));
}

sf::Vector2f
BezierRailSegment::cubicBezier(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t) {
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;

    return p0 * (uu * u) + p1 * (3.0f * uu * t) + p2 * (3.0f * u * tt) + p3 * (tt * t);
}

sf::Vector2f BezierRailSegment::cubicBezierDerivative(
    sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t) {
    const float u = 1.0f - t;
    return (p1 - p0) * (3.0f * u * u) + (p2 - p1) * (6.0f * u * t) + (p3 - p2) * (3.0f * t * t);
}

void BezierRailSegment::rebuildGeometry() const {
    std::array<sf::Vector2f, kBezierSampleCount + 1> points{};
    for (std::size_t index = 0; index < points.size(); ++index) {
        const float t = static_cast<float>(index) / static_cast<float>(points.size() - 1);
        points[index] = cubicBezier(m_p0, m_p1, m_p2, m_p3, t);
    }

    m_geometry = buildThickPolyline(points, getThickness(), getColor());
}
} // namespace view::rails
