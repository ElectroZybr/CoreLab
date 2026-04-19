#include "view/BlockView.h"

#include <array>
#include <cmath>
#include <numbers>
#include <utility>

namespace {
constexpr int kCornerPointCount = 16;
}

namespace view {
BlockView::BlockView(const sf::Font* font, sf::Vector2f position, sf::Vector2f size) {
    m_font = font;
    setBlockSize(size);
    ViewNode::setPosition(position);
}

void BlockView::setPosition(sf::Vector2f position) {
    ViewNode::setPosition(position);
}

sf::FloatRect BlockView::getBounds() const {
    return {getWorldPosition(), m_size};
}

bool BlockView::isInDragHandle(sf::Vector2f worldPoint) const {
    const sf::FloatRect dragHandleBounds{getWorldPosition(), {m_size.x, m_headerLayout.dragHandleHeight}};
    return dragHandleBounds.contains(worldPoint);
}

void BlockView::setDragState(bool hovered, bool dragging) {
    m_dragHovered = hovered;
    m_dragging = dragging;

    const sf::Color overlayColor = m_dragging
                                       ? m_dragHandleActiveColor
                                       : (m_dragHovered ? m_dragHandleHoverColor : sf::Color::Transparent);
    m_dragHandleOverlay.setFillColor(overlayColor);
}

void BlockView::setFont(const sf::Font* font) {
    m_font = font;
    rebuildBlockText();
    layoutBlock();
}

PortView& BlockView::addPort(std::string id,
                             PortKind kind,
                             PortDirection direction,
                             PayloadKind payloadKind) {
    auto port = std::make_unique<PortView>(std::move(id));
    port->setKind(kind);
    port->setDirection(direction);
    port->setPayloadKind(payloadKind);
    addChild(*port);
    m_ports.push_back(std::move(port));
    return *m_ports.back();
}

PortView* BlockView::findPort(std::string_view id) {
    for (const std::unique_ptr<PortView>& port : m_ports) {
        if (port && port->getId() == id) {
            return port.get();
        }
    }

    return nullptr;
}

const PortView* BlockView::findPort(std::string_view id) const {
    for (const std::unique_ptr<PortView>& port : m_ports) {
        if (port && port->getId() == id) {
            return port.get();
        }
    }

    return nullptr;
}

void BlockView::buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius) {
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

void BlockView::setBlockSize(sf::Vector2f size) {
    m_size = size;
    rebuildBlockGeometry();
    rebuildBlockText();
    layoutBlock();
}

void BlockView::setTitle(std::string title) {
    m_title = std::move(title);
    rebuildBlockText();
    layoutBlock();
}

void BlockView::setSubtitle(std::string subtitle) {
    m_subtitle = std::move(subtitle);
    rebuildBlockText();
    layoutBlock();
}

void BlockView::setHeaderLayout(HeaderLayout headerLayout) {
    m_headerLayout = headerLayout;
    rebuildBlockGeometry();
    rebuildBlockText();
    layoutBlock();
}

void BlockView::rebuildBlockGeometry() {
    m_container.setFillColor(m_containerFillColor);
    m_container.setOutlineThickness(0.0f);
    buildRoundedRect(m_container, m_size, m_cornerRadius);
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(m_containerOutlineColor);

    m_dragHandleOverlay.setOutlineThickness(0.0f);
    buildRoundedRect(m_dragHandleOverlay, {m_size.x, m_headerLayout.dragHandleHeight}, m_cornerRadius);
    m_dragHandleOverlay.setFillColor(sf::Color::Transparent);
}

void BlockView::rebuildBlockText() {
    m_titleText.reset();
    m_subtitleText.reset();

    if (!m_font) {
        return;
    }

    if (!m_title.empty()) {
        m_titleText.emplace(*m_font, m_title, m_headerLayout.titleTextSize);
        m_titleText->setFillColor(m_titleColor);
    }

    if (!m_subtitle.empty()) {
        m_subtitleText.emplace(*m_font, m_subtitle, m_headerLayout.subtitleTextSize);
        m_subtitleText->setFillColor(m_subtitleColor);
    }
}

void BlockView::layoutBlock() {
    const sf::Vector2f worldPosition = getWorldPosition();

    m_container.setPosition(worldPosition);
    m_dragHandleOverlay.setPosition(worldPosition);

    if (m_titleText) {
        const sf::FloatRect bounds = m_titleText->getLocalBounds();
        m_titleText->setPosition({worldPosition.x + (m_size.x - bounds.size.x) * 0.5f - bounds.position.x,
                                  worldPosition.y + m_headerLayout.titleY - bounds.position.y});
    }

    if (m_subtitleText) {
        const sf::FloatRect bounds = m_subtitleText->getLocalBounds();
        m_subtitleText->setPosition({worldPosition.x + (m_size.x - bounds.size.x) * 0.5f - bounds.position.x,
                                     worldPosition.y + m_headerLayout.subtitleY - bounds.position.y});
    }
}

void BlockView::onPositionChanged() {
    layoutBlock();
}

void BlockView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);
    target.draw(m_dragHandleOverlay, states);

    if (m_titleText) {
        target.draw(*m_titleText, states);
    }

    if (m_subtitleText) {
        target.draw(*m_subtitleText, states);
    }

    drawBlockContent(target, states);
    drawChildren(target, states);
}
} // namespace view
