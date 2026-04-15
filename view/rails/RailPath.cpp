#include "view/rails/RailPath.h"

#include <algorithm>

namespace view::rails {
RailPath::RailPath(RailStyle style) : m_style(style) {
}

void RailPath::setStyle(RailStyle style) {
    m_style = style;
}

void RailPath::appendStraight(sf::Vector2f startPoint, sf::Vector2f endPoint) {
    m_segments.push_back(
        std::make_unique<StraightRailSegment>(startPoint, endPoint, m_style.thickness, m_style.color));
}

void RailPath::appendArc(sf::Vector2f center, float radius, float startAngleRadians, float endAngleRadians) {
    m_segments.push_back(std::make_unique<ArcRailSegment>(
        center, radius, startAngleRadians, endAngleRadians, m_style.thickness, m_style.color));
}

void RailPath::appendPath(RailPath path) {
    for (std::unique_ptr<RailSegment>& segment : path.m_segments) {
        m_segments.push_back(std::move(segment));
    }
}

float RailPath::getLength() const {
    float totalLength = 0.0f;
    for (const std::unique_ptr<RailSegment>& segment : m_segments) {
        totalLength += segment->getLength();
    }
    return totalLength;
}

sf::Vector2f RailPath::getStartPoint() const {
    if (m_segments.empty()) {
        return {0.0f, 0.0f};
    }

    return m_segments.front()->getStartPoint();
}

sf::Vector2f RailPath::getEndPoint() const {
    if (m_segments.empty()) {
        return {0.0f, 0.0f};
    }

    return m_segments.back()->getEndPoint();
}

sf::Vector2f RailPath::samplePoint(float distance) const {
    if (m_segments.empty()) {
        return {0.0f, 0.0f};
    }

    float remainingDistance = std::clamp(distance, 0.0f, getLength());
    for (const std::unique_ptr<RailSegment>& segment : m_segments) {
        const float segmentLength = segment->getLength();
        if (remainingDistance <= segmentLength) {
            return segment->samplePoint(remainingDistance);
        }

        remainingDistance -= segmentLength;
    }

    return m_segments.back()->getEndPoint();
}

sf::Vector2f RailPath::sampleTangent(float distance) const {
    if (m_segments.empty()) {
        return {0.0f, 0.0f};
    }

    float remainingDistance = std::clamp(distance, 0.0f, getLength());
    for (const std::unique_ptr<RailSegment>& segment : m_segments) {
        const float segmentLength = segment->getLength();
        if (remainingDistance <= segmentLength) {
            return segment->sampleTangent(remainingDistance);
        }

        remainingDistance -= segmentLength;
    }

    return m_segments.back()->sampleTangent(m_segments.back()->getLength());
}

void RailPath::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const std::unique_ptr<RailSegment>& segment : m_segments) {
        target.draw(*segment, states);
    }
}
} // namespace view::rails
