#include "view/BusView.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "view/CacheLineView.h"

namespace {
const sf::Color kBusColor(116, 134, 165);
const sf::Color kHighlightBusColor(247, 214, 92, 210);

sf::Vector2f toCenter(sf::Vector2f topLeft) {
    return {topLeft.x + view::CacheLineView::kWidth * 0.5f, topLeft.y + view::CacheLineView::kHeight * 0.5f};
}

sf::Vector2f toTopLeft(sf::Vector2f center) {
    return {center.x - view::CacheLineView::kWidth * 0.5f, center.y - view::CacheLineView::kHeight * 0.5f};
}

view::rails::RailPath
buildBusPath(sf::Vector2f startCenter, sf::Vector2f endCenter, view::rails::RailStyle style, float turnRadius) {
    view::rails::RailPath path(style);

    const float horizontalSpan = std::abs(startCenter.x - endCenter.x);
    const float verticalSpan = std::abs(startCenter.y - endCenter.y);
    if (horizontalSpan <= 1.0f || verticalSpan <= 1.0f || endCenter.x >= startCenter.x) {
        path.appendStraight(startCenter, endCenter);
        return path;
    }

    const float radius = std::max(1.0f, std::min({turnRadius, horizontalSpan * 0.25f, verticalSpan * 0.5f}));
    const float columnX = (startCenter.x + endCenter.x) * 0.5f;
    const sf::Vector2f firstTurnEntry{columnX + radius, startCenter.y};

    if (std::abs(firstTurnEntry.x - startCenter.x) > 0.001f) {
        path.appendStraight(startCenter, firstTurnEntry);
    }

    if (endCenter.y < startCenter.y) {
        const sf::Vector2f firstTurnCenter{columnX + radius, startCenter.y - radius};
        const sf::Vector2f verticalStart{columnX, startCenter.y - radius};
        const sf::Vector2f verticalEnd{columnX, endCenter.y + radius};
        const sf::Vector2f secondTurnCenter{columnX - radius, endCenter.y + radius};
        const sf::Vector2f secondTurnExit{columnX - radius, endCenter.y};

        path.appendArc(firstTurnCenter, radius, std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float>);
        if (std::abs(verticalStart.y - verticalEnd.y) > 0.001f) {
            path.appendStraight(verticalStart, verticalEnd);
        }
        path.appendArc(secondTurnCenter, radius, 0.0f, -std::numbers::pi_v<float> * 0.5f);
        if (std::abs(secondTurnExit.x - endCenter.x) > 0.001f) {
            path.appendStraight(secondTurnExit, endCenter);
        }
    } else {
        const sf::Vector2f firstTurnCenter{columnX + radius, startCenter.y + radius};
        const sf::Vector2f verticalStart{columnX, startCenter.y + radius};
        const sf::Vector2f verticalEnd{columnX, endCenter.y - radius};
        const sf::Vector2f secondTurnCenter{columnX - radius, endCenter.y - radius};
        const sf::Vector2f secondTurnExit{columnX - radius, endCenter.y};

        path.appendArc(firstTurnCenter, radius, -std::numbers::pi_v<float> * 0.5f, -std::numbers::pi_v<float>);
        if (std::abs(verticalStart.y - verticalEnd.y) > 0.001f) {
            path.appendStraight(verticalStart, verticalEnd);
        }
        path.appendArc(secondTurnCenter, radius, 0.0f, std::numbers::pi_v<float> * 0.5f);
        if (std::abs(secondTurnExit.x - endCenter.x) > 0.001f) {
            path.appendStraight(secondTurnExit, endCenter);
        }
    }

    return path;
}

view::rails::RailPath buildDirectedBusPath(sf::Vector2f startCenter,
                                           sf::Vector2f endCenter,
                                           view::rails::RailDirection endDirection,
                                           view::rails::RailStyle style,
                                           float turnRadius) {
    if (endDirection != view::rails::RailDirection::Down) {
        return view::rails::RailBuilder::orthogonal({startCenter, view::rails::RailDirection::Left},
                                                    {endCenter, endDirection},
                                                    turnRadius,
                                                    style);
    }

    view::rails::RailPath path(style);

    const float horizontalSpan = startCenter.x - endCenter.x;
    if (horizontalSpan <= turnRadius * 4.0f) {
        return view::rails::RailBuilder::orthogonal({startCenter, view::rails::RailDirection::Left},
                                                    {endCenter, endDirection},
                                                    turnRadius,
                                                    style);
    }

    const float radius = std::max(1.0f, std::min(turnRadius, horizontalSpan * 0.2f));
    const float firstCornerX = std::min(startCenter.x - radius * 2.0f, endCenter.x + radius * 3.0f);
    const bool targetIsAbove = endCenter.y < startCenter.y;
    const float middleY = endCenter.y - radius;

    const sf::Vector2f firstStraightEnd{firstCornerX + radius, startCenter.y};
    if (std::abs(firstStraightEnd.x - startCenter.x) > 0.001f) {
        path.appendStraight(startCenter, firstStraightEnd);
    }

    if (targetIsAbove) {
        path.appendArc({firstCornerX + radius, startCenter.y - radius},
                       radius,
                       std::numbers::pi_v<float> * 0.5f,
                       std::numbers::pi_v<float>);

        const sf::Vector2f verticalStart{firstCornerX, startCenter.y - radius};
        const sf::Vector2f verticalEnd{firstCornerX, middleY + radius};
        if (std::abs(verticalStart.y - verticalEnd.y) > 0.001f) {
            path.appendStraight(verticalStart, verticalEnd);
        }

        path.appendArc({firstCornerX - radius, middleY + radius},
                       radius,
                       0.0f,
                       -std::numbers::pi_v<float> * 0.5f);
    } else {
        path.appendArc({firstCornerX + radius, startCenter.y + radius},
                       radius,
                       -std::numbers::pi_v<float> * 0.5f,
                       -std::numbers::pi_v<float>);

        const sf::Vector2f verticalStart{firstCornerX, startCenter.y + radius};
        const sf::Vector2f verticalEnd{firstCornerX, middleY - radius};
        if (std::abs(verticalStart.y - verticalEnd.y) > 0.001f) {
            path.appendStraight(verticalStart, verticalEnd);
        }

        path.appendArc({firstCornerX - radius, middleY - radius},
                       radius,
                       0.0f,
                       std::numbers::pi_v<float> * 0.5f);
    }

    const sf::Vector2f middleStart{firstCornerX - radius, middleY};
    const sf::Vector2f middleEnd{endCenter.x + radius, middleY};
    if (std::abs(middleStart.x - middleEnd.x) > 0.001f) {
        path.appendStraight(middleStart, middleEnd);
    }

    path.appendArc({endCenter.x + radius, middleY + radius},
                   radius,
                   -std::numbers::pi_v<float> * 0.5f,
                   -std::numbers::pi_v<float>);

    return path;
}
} // namespace

namespace view {
BusView::BusView(float thickness, float turnRadius)
    : m_thickness(thickness), m_turnRadius(turnRadius), m_style{thickness, kBusColor},
      m_highlightStyle{thickness + 1.0f, kHighlightBusColor}, m_path(m_style), m_highlightPath(m_highlightStyle) {
}

void BusView::setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft) {
    setCenterEndpoints(toCenter(startTopLeft), toCenter(endTopLeft));
}

void BusView::setCenterEndpoints(sf::Vector2f startCenter, sf::Vector2f endCenter) {
    m_path = buildBusPath(startCenter, endCenter, m_style, m_turnRadius);
    m_highlightPath = buildBusPath(startCenter, endCenter, m_highlightStyle, m_turnRadius);
    m_visible = !m_path.isEmpty();
}

void BusView::setEndpoints(
    sf::Vector2f startTopLeft, sf::Vector2f endTopLeft, rails::RailDirection endDirection) {
    setCenterEndpoints(toCenter(startTopLeft), toCenter(endTopLeft), endDirection);
}

void BusView::setCenterEndpoints(
    sf::Vector2f startCenter, sf::Vector2f endCenter, rails::RailDirection endDirection) {
    m_path = buildDirectedBusPath(startCenter, endCenter, endDirection, m_style, m_turnRadius);
    m_highlightPath = buildDirectedBusPath(startCenter, endCenter, endDirection, m_highlightStyle, m_turnRadius);
    m_visible = !m_path.isEmpty();
}

void BusView::clear() {
    m_visible = false;
    m_path = rails::RailPath(m_style);
    m_highlightPath = rails::RailPath(m_highlightStyle);
}

sf::Vector2f BusView::sampleTopLeft(float progress) const {
    if (!m_visible || m_path.isEmpty()) {
        return {0.0f, 0.0f};
    }

    const float distance = m_path.getLength() * std::clamp(progress, 0.0f, 1.0f);
    return toTopLeft(m_path.samplePoint(distance));
}

void BusView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_visible) {
        return;
    }

    target.draw(m_path, states);
    if (m_highlighted) {
        target.draw(m_highlightPath, states);
    }
}
} // namespace view
