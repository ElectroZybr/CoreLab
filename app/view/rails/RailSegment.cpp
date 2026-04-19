#include "view/rails/RailSegment.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace view::rails {
RailSegment::RailSegment(float thickness, sf::Color color)
    : m_geometry(sf::PrimitiveType::TriangleStrip), m_color(color) {
    setThickness(thickness);
}

void RailSegment::setThickness(float thickness) {
    m_thickness = std::max(thickness, 1.0f);
    markDirty();
}

void RailSegment::setColor(sf::Color color) {
    m_color = color;
    markDirty();
}

float RailSegment::clampDistance(float distance) const {
    return std::clamp(distance, 0.0f, getLength());
}

sf::Vector2f RailSegment::normalizeOrZero(sf::Vector2f vector) {
    const float lengthSquared = vector.x * vector.x + vector.y * vector.y;
    if (lengthSquared <= 0.0f) {
        return {0.0f, 0.0f};
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return {vector.x * inverseLength, vector.y * inverseLength};
}

sf::VertexArray
RailSegment::buildThickPolyline(std::span<const sf::Vector2f> points, float thickness, sf::Color color) {
    sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
    if (points.size() < 2) {
        return strip;
    }

    for (std::size_t index = 0; index < points.size(); ++index) {
        sf::Vector2f tangent{0.0f, 0.0f};
        if (index == 0) {
            tangent = points[1] - points[0];
        } else if (index + 1 == points.size()) {
            tangent = points[index] - points[index - 1];
        } else {
            tangent = points[index + 1] - points[index - 1];
        }

        const sf::Vector2f normal = normalizeOrZero({-tangent.y, tangent.x}) * (thickness * 0.5f);
        strip.append(sf::Vertex(points[index] + normal, color));
        strip.append(sf::Vertex(points[index] - normal, color));
    }

    return strip;
}

void RailSegment::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (m_dirty) {
        rebuildGeometry();
        m_dirty = false;
    }

    target.draw(m_geometry, states);
}
} // namespace view::rails
