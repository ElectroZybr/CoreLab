#include "view/CpuView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr sf::Vector2f kSize{620.0f, 330.0f};
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr float kDragHandleHeight = 76.0f;
constexpr float kDragMarkWidth = 60.0f;
constexpr float kDragMarkHeight = 5.0f;
constexpr float kDragMarkGap = 8.0f;
constexpr float kContentPadding = 28.0f;
constexpr float kSectionGap = 18.0f;
constexpr float kTopContentY = 110.0f;
constexpr float kSectionCornerRadius = 16.0f;
constexpr unsigned int kTitleTextSize = 30;
constexpr unsigned int kSummaryTextSize = 18;
constexpr unsigned int kSectionTextSize = 22;
constexpr std::array<const char*, 4> kSectionLabels{"Control", "ALU", "SIMD", "Registers"};

const sf::Color kContainerFillColor(36, 44, 60);
const sf::Color kContainerOutlineColor(88, 112, 150);
const sf::Color kSectionFillColors[] = {
    sf::Color(53, 67, 91),
    sf::Color(61, 80, 109),
    sf::Color(67, 90, 122),
    sf::Color(49, 60, 82),
};
const sf::Color kSectionOutlineColor(116, 134, 165);
const sf::Color kTitleColor(233, 245, 255);
const sf::Color kSummaryColor(182, 203, 229);
const sf::Color kSectionTextColor(220, 233, 249);
const sf::Color kDragHandleHoverColor(148, 172, 210, 34);
const sf::Color kDragHandleActiveColor(186, 214, 255, 54);
const sf::Color kDragMarkIdleColor(116, 134, 165, 120);
const sf::Color kDragMarkHoverColor(183, 210, 244, 170);
const sf::Color kDragMarkActiveColor(225, 239, 255, 220);

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
CpuView::CpuView(const sf::Font* font, sf::Vector2f position) : m_font(font), m_position(position), m_size(kSize) {
    rebuildGeometry();
    rebuildText();
    layout();
}

void CpuView::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

sf::FloatRect CpuView::getBounds() const {
    return {m_position, m_size};
}

bool CpuView::isInDragHandle(sf::Vector2f worldPoint) const {
    const sf::FloatRect dragHandleBounds{m_position, {m_size.x, kDragHandleHeight}};
    return dragHandleBounds.contains(worldPoint);
}

void CpuView::setDragState(bool hovered, bool dragging) {
    m_dragHovered = hovered;
    m_dragging = dragging;

    const sf::Color overlayColor = m_dragging
                                       ? kDragHandleActiveColor
                                       : (m_dragHovered ? kDragHandleHoverColor : sf::Color::Transparent);
    m_dragHandleOverlay.setFillColor(overlayColor);

    const sf::Color markColor =
        m_dragging ? kDragMarkActiveColor : (m_dragHovered ? kDragMarkHoverColor : kDragMarkIdleColor);
    for (sf::RectangleShape& mark : m_dragHandleMarks) {
        mark.setFillColor(markColor);
    }
}

void CpuView::setFont(const sf::Font* font) {
    m_font = font;
    rebuildText();
    layout();
}

void CpuView::rebuildGeometry() {
    m_container.setFillColor(kContainerFillColor);
    m_container.setOutlineThickness(0.0f);
    buildRoundedRect(m_container, m_size, kCornerRadius);
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(kContainerOutlineColor);

    m_dragHandleOverlay.setOutlineThickness(0.0f);
    buildRoundedRect(m_dragHandleOverlay, {m_size.x, kDragHandleHeight}, kCornerRadius);
    m_dragHandleOverlay.setFillColor(sf::Color::Transparent);

    m_dragHandleMarks.assign(3, sf::RectangleShape{{kDragMarkWidth, kDragMarkHeight}});
    for (sf::RectangleShape& mark : m_dragHandleMarks) {
        mark.setOrigin({kDragMarkWidth * 0.5f, kDragMarkHeight * 0.5f});
        mark.setFillColor(kDragMarkIdleColor);
    }

    for (std::size_t index = 0; index < m_sections.size(); ++index) {
        m_sections[index].setOutlineThickness(0.0f);
        buildRoundedRect(m_sections[index], {0.0f, 0.0f}, kSectionCornerRadius);
        m_sections[index].setOutlineThickness(3.0f);
        m_sections[index].setOutlineColor(kSectionOutlineColor);
        m_sections[index].setFillColor(kSectionFillColors[index]);
    }
}

void CpuView::rebuildText() {
    m_titleText.reset();
    m_summaryText.reset();
    for (std::optional<sf::Text>& sectionText : m_sectionTexts) {
        sectionText.reset();
    }

    if (!m_font) {
        return;
    }

    m_titleText.emplace(*m_font, "CPU", kTitleTextSize);
    m_titleText->setFillColor(kTitleColor);

    m_summaryText.emplace(*m_font, "Frontend  |  Execution  |  State", kSummaryTextSize);
    m_summaryText->setFillColor(kSummaryColor);

    for (std::size_t index = 0; index < m_sectionTexts.size(); ++index) {
        m_sectionTexts[index].emplace(*m_font, kSectionLabels[index], kSectionTextSize);
        m_sectionTexts[index]->setFillColor(kSectionTextColor);
    }
}

void CpuView::layout() {
    m_container.setPosition(m_position);
    m_dragHandleOverlay.setPosition(m_position);

    const float marksCenterX = m_position.x + m_size.x * 0.5f;
    const float marksStartY = m_position.y + 22.0f;
    for (std::size_t index = 0; index < m_dragHandleMarks.size(); ++index) {
        m_dragHandleMarks[index].setPosition(
            {marksCenterX, marksStartY + static_cast<float>(index) * (kDragMarkHeight + kDragMarkGap)});
    }

    if (m_titleText) {
        m_titleText->setPosition({m_position.x + 24.0f, m_position.y + 18.0f});
    }

    if (m_summaryText) {
        m_summaryText->setPosition({m_position.x + 24.0f, m_position.y + 56.0f});
    }

    const float sectionWidth = (m_size.x - kContentPadding * 2.0f - kSectionGap) * 0.5f;
    const float sectionHeight = (m_size.y - kTopContentY - kContentPadding - kSectionGap) * 0.5f;

    for (std::size_t index = 0; index < m_sections.size(); ++index) {
        const std::size_t column = index % 2;
        const std::size_t row = index / 2;
        const sf::Vector2f sectionPosition{
            m_position.x + kContentPadding + static_cast<float>(column) * (sectionWidth + kSectionGap),
            m_position.y + kTopContentY + static_cast<float>(row) * (sectionHeight + kSectionGap)};

        m_sections[index].setOutlineThickness(0.0f);
        buildRoundedRect(m_sections[index], {sectionWidth, sectionHeight}, kSectionCornerRadius);
        m_sections[index].setOutlineThickness(3.0f);
        m_sections[index].setPosition(sectionPosition);

        if (m_sectionTexts[index]) {
            sf::Text& text = *m_sectionTexts[index];
            const sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(
                {sectionPosition.x + (sectionWidth - bounds.size.x) * 0.5f - bounds.position.x,
                 sectionPosition.y + (sectionHeight - bounds.size.y) * 0.5f - bounds.position.y});
        }
    }
}

void CpuView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);
    target.draw(m_dragHandleOverlay, states);

    for (const sf::RectangleShape& mark : m_dragHandleMarks) {
        target.draw(mark, states);
    }

    if (m_titleText) {
        target.draw(*m_titleText, states);
    }

    if (m_summaryText) {
        target.draw(*m_summaryText, states);
    }

    for (std::size_t index = 0; index < m_sections.size(); ++index) {
        target.draw(m_sections[index], states);
        if (m_sectionTexts[index]) {
            target.draw(*m_sectionTexts[index], states);
        }
    }
}
} // namespace view
