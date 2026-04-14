#include "CacheLine.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr float kCornerRadius = 16.0f;
constexpr int kCornerPointCount = 16;
constexpr float kDividerWidth = 2.0f;
constexpr unsigned int kCellTextSize = 24;

constexpr std::array<const char*, CacheLine::kFloatCount> kDefaultLabels{
    "x", "y", "z", "vx", "vy", "vz", "...", "..."};

void buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius) {
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
} // namespace

CacheLine::CacheLine(const sf::Font* font) : m_font(font) {
    buildRoundedRect(m_container, {kWidth, kHeight}, kCornerRadius);
    m_container.setFillColor(sf::Color(200, 210, 223));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(70, 97, 138));

    for (sf::RectangleShape& divider : m_dividers) {
        divider.setSize({kDividerWidth, kHeight});
        divider.setFillColor(sf::Color(70, 97, 138));
    }

    rebuildText();
    layout();
}

void CacheLine::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

void CacheLine::setFont(const sf::Font* font) {
    m_font = font;
    rebuildText();
    layout();
}

void CacheLine::rebuildText() {
    for (std::optional<sf::Text>& cellText : m_cellTexts) {
        cellText.reset();
    }

    if (!m_font) {
        return;
    }

    for (std::size_t index = 0; index < m_cellTexts.size(); ++index) {
        m_cellTexts[index].emplace(*m_font, kDefaultLabels[index], kCellTextSize);
        m_cellTexts[index]->setFillColor(sf::Color(27, 40, 67));
    }
}

void CacheLine::layout() {
    m_container.setPosition(m_position);

    const float floatWidth = kWidth / static_cast<float>(kFloatCount);
    for (std::size_t index = 0; index < m_dividers.size(); ++index) {
        const float x = m_position.x + static_cast<float>(index + 1) * floatWidth - kDividerWidth * 0.5f;
        m_dividers[index].setPosition({x, m_position.y});
    }

    for (std::size_t index = 0; index < m_cellTexts.size(); ++index) {
        if (!m_cellTexts[index]) {
            continue;
        }

        const sf::FloatRect bounds = m_cellTexts[index]->getLocalBounds();
        const float cellX = m_position.x + static_cast<float>(index) * floatWidth;
        m_cellTexts[index]->setPosition(
            {cellX + (floatWidth - bounds.size.x) * 0.5f - bounds.position.x,
             m_position.y + (kHeight - bounds.size.y) * 0.5f - bounds.position.y});
    }
}

void CacheLine::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);

    for (const sf::RectangleShape& divider : m_dividers) {
        target.draw(divider, states);
    }

    for (const std::optional<sf::Text>& cellText : m_cellTexts) {
        if (cellText) {
            target.draw(*cellText, states);
        }
    }
}
