#include "view/CacheLineView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr float kCornerRadius = 16.0f;
constexpr int kCornerPointCount = 16;
constexpr float kDividerWidth = 2.0f;
constexpr unsigned int kCellTextSize = 16;
const sf::Color kHighlightColor(247, 214, 92);

constexpr std::array<const char*, view::CacheLineView::kFloatCount> kDefaultLabels{
    "x[0]",  "y[0]",  "z[0]",  "vx[0]", "vy[0]", "vz[0]",
    "x[1]",  "y[1]",  "z[1]",  "vx[1]", "vy[1]", "vz[1]",
    "x[2]",  "y[2]",  "z[2]",  "vx[2]"}; 

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

namespace view {
CacheLineView::CacheLineView(const sf::Font* font, sf::Vector2f position)
    : m_font(font), m_position(position) {
    for (std::size_t index = 0; index < m_labels.size(); ++index) {
        m_labels[index] = kDefaultLabels[index];
    }

    m_container.setFillColor(sf::Color(200, 210, 223));
    m_container.setOutlineThickness(0.0f);
    buildRoundedRect(m_container, {kWidth, kHeight}, kCornerRadius);
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(70, 97, 138));

    for (sf::RectangleShape& divider : m_dividers) {
        divider.setSize({kDividerWidth, kHeight});
        divider.setFillColor(sf::Color(70, 97, 138));
    }

    for (sf::RectangleShape& highlight : m_cellHighlights) {
        highlight.setFillColor(sf::Color::Transparent);
    }

    rebuildText();
    layout();
}

void CacheLineView::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

void CacheLineView::setFont(const sf::Font* font) {
    m_font = font;
    rebuildText();
    layout();
}

void CacheLineView::setCellLabels(const std::array<std::string, kFloatCount>& labels) {
    m_labels = labels;
    rebuildText();
    layout();
}

void CacheLineView::setHighlightedCell(std::optional<std::size_t> cellIndex, float intensity) {
    m_highlightIntensities.fill(0.0f);
    if (cellIndex.has_value()) {
        m_highlightIntensities[*cellIndex] = std::clamp(intensity, 0.0f, 1.0f);
    }
    layout();
}

void CacheLineView::setHighlightedCells(const std::array<float, kFloatCount>& intensities) {
    m_highlightIntensities = intensities;
    layout();
}

void CacheLineView::rebuildText() {
    for (std::optional<sf::Text>& cellText : m_cellTexts) {
        cellText.reset();
    }

    if (!m_font) {
        return;
    }

    for (std::size_t index = 0; index < m_cellTexts.size(); ++index) {
        if (m_labels[index].empty()) {
            continue;
        }

        m_cellTexts[index].emplace(*m_font, m_labels[index], kCellTextSize);
        m_cellTexts[index]->setFillColor(sf::Color(27, 40, 67));
    }
}

void CacheLineView::layout() {
    m_container.setPosition(m_position);

    const float floatWidth = kWidth / static_cast<float>(kFloatCount);
    for (std::size_t index = 0; index < m_cellHighlights.size(); ++index) {
        const float inset = 3.0f;
        const float cellX = m_position.x + static_cast<float>(index) * floatWidth;
        m_cellHighlights[index].setPosition({cellX + inset, m_position.y + inset});
        m_cellHighlights[index].setSize({floatWidth - inset * 2.0f, kHeight - inset * 2.0f});
        const float intensity = std::clamp(m_highlightIntensities[index], 0.0f, 1.0f);
        m_cellHighlights[index].setFillColor(
            sf::Color(kHighlightColor.r, kHighlightColor.g, kHighlightColor.b,
                      static_cast<std::uint8_t>(intensity > 0.0f ? 40.0f + 120.0f * intensity : 0.0f)));
    }

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

void CacheLineView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);
    for (std::size_t index = 0; index < m_cellHighlights.size(); ++index) {
        if (m_highlightIntensities[index] > 0.0f) {
            target.draw(m_cellHighlights[index], states);
        }
    }

    for (const sf::RectangleShape& divider : m_dividers) {
        target.draw(divider, states);
    }

    for (const std::optional<sf::Text>& cellText : m_cellTexts) {
        if (cellText) {
            target.draw(*cellText, states);
        }
    }
}
} // namespace view
