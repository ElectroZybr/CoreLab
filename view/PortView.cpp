#include "view/PortView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr int kCornerPointCount = 16;
}

namespace view {
PortView::PortView(std::string id) : m_id(std::move(id)) {
    rebuildGeometry();
}

void PortView::setId(std::string id) {
    m_id = std::move(id);
}

void PortView::setDirection(PortDirection direction) {
    m_direction = direction;
}

void PortView::setKind(PortKind kind) {
    m_kind = kind;
}

void PortView::setPayloadKind(PayloadKind payloadKind) {
    m_payloadKind = payloadKind;
}

void PortView::setLocalAnchor(sf::Vector2f anchor) {
    setPosition(anchor);
}

void PortView::setSize(sf::Vector2f size) {
    m_size = size;
    rebuildGeometry();
}

void PortView::setColors(sf::Color fillColor, sf::Color outlineColor) {
    m_fillColor = fillColor;
    m_outlineColor = outlineColor;
    rebuildGeometry();
}

void PortView::buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius) {
    constexpr float halfPi = std::numbers::pi_v<float> * 0.5f;
    constexpr std::array<float, 4> arcOffsets{
        std::numbers::pi_v<float>, std::numbers::pi_v<float> * 1.5f, 0.0f, halfPi};

    const std::array<sf::Vector2f, 4> centers{sf::Vector2f(radius, radius),
                                              sf::Vector2f(size.x - radius, radius),
                                              sf::Vector2f(size.x - radius, size.y - radius),
                                              sf::Vector2f(radius, size.y - radius)};

    shape.setPointCount(arcOffsets.size() * kCornerPointCount);
    std::size_t pointIndex = 0;
    for (std::size_t cornerIndex = 0; cornerIndex < centers.size(); ++cornerIndex) {
        for (int step = 0; step < kCornerPointCount; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(kCornerPointCount - 1);
            const float angle = arcOffsets[cornerIndex] + t * halfPi;
            shape.setPoint(pointIndex++,
                           {centers[cornerIndex].x + std::cos(angle) * radius,
                            centers[cornerIndex].y + std::sin(angle) * radius});
        }
    }
}

void PortView::rebuildGeometry() {
    m_shape.setOutlineThickness(0.0f);
    buildRoundedRect(m_shape, m_size, std::min(m_size.x, m_size.y) * 0.42f);
    m_shape.setOutlineThickness(3.0f);
    m_shape.setFillColor(m_fillColor);
    m_shape.setOutlineColor(m_outlineColor);
    m_shape.setOrigin(m_size * 0.5f);

    switch (m_direction) {
    case PortDirection::Left:
    case PortDirection::Right:
        m_shape.setRotation(sf::degrees(0.0f));
        break;
    case PortDirection::Up:
    case PortDirection::Down:
        m_shape.setRotation(sf::degrees(90.0f));
        break;
    }
}

void PortView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    sf::RenderStates localStates = states;
    localStates.transform.translate(getWorldAnchor());
    target.draw(m_shape, localStates);
}
} // namespace view
