#include "view/BusView.h"

#include <cmath>
#include <numbers>

#include "view/CacheLineView.h"

namespace {
const sf::Color kBusColor(116, 134, 165);

sf::Vector2f toCenter(sf::Vector2f topLeft) {
    return {topLeft.x + view::CacheLineView::kWidth * 0.5f, topLeft.y + view::CacheLineView::kHeight * 0.5f};
}
} // namespace

namespace view {
BusView::BusView(float thickness) : m_thickness(thickness) {
    m_body.setFillColor(kBusColor);
    m_body.setOrigin({0.0f, m_thickness * 0.5f});

    m_startCap.setRadius(m_thickness * 0.5f);
    m_startCap.setOrigin({m_thickness * 0.5f, m_thickness * 0.5f});
    m_startCap.setFillColor(kBusColor);

    m_endCap = m_startCap;
}

void BusView::setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft) {
    const sf::Vector2f startCenter = toCenter(startTopLeft);
    const sf::Vector2f endCenter = toCenter(endTopLeft);
    const sf::Vector2f delta = endCenter - startCenter;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    if (length <= 0.0f) {
        m_visible = false;
        return;
    }

    const float angleDegrees = std::atan2(delta.y, delta.x) * 180.0f / std::numbers::pi_v<float>;
    m_body.setPosition(startCenter);
    m_body.setSize({length, m_thickness});
    m_body.setRotation(sf::degrees(angleDegrees));

    m_startCap.setPosition(startCenter);
    m_endCap.setPosition(endCenter);
    m_visible = true;
}

void BusView::clear() {
    m_visible = false;
}

void BusView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_visible) {
        return;
    }

    target.draw(m_body, states);
    target.draw(m_startCap, states);
    target.draw(m_endCap, states);
}
} // namespace view
