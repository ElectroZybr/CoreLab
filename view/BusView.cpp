#include "view/BusView.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "view/CacheLineView.h"

namespace {
const sf::Color kBusColor(116, 134, 165);

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
} // namespace

namespace view {
BusView::BusView(float thickness, float turnRadius)
    : m_thickness(thickness), m_turnRadius(turnRadius), m_style{thickness, kBusColor}, m_path(m_style) {
}

void BusView::setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft) {
    const sf::Vector2f startCenter = toCenter(startTopLeft);
    const sf::Vector2f endCenter = toCenter(endTopLeft);
    m_path = buildBusPath(startCenter, endCenter, m_style, m_turnRadius);
    m_visible = !m_path.isEmpty();
}

void BusView::clear() {
    m_visible = false;
    m_path = rails::RailPath(m_style);
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
}
} // namespace view
