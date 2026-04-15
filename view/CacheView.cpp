#include "view/CacheView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr float kHorizontalPadding = 34.0f;
constexpr float kTopPadding = 18.0f;
constexpr float kBottomPadding = 28.0f;
constexpr float kTitleOffsetX = 24.0f;
constexpr float kTitleOffsetY = 18.0f;
constexpr float kSummaryOffsetY = 56.0f;
constexpr float kSlotsOffsetY = 98.0f;
constexpr float kDragHandleHeight = kSlotsOffsetY - 8.0f;
constexpr float kDragMarkWidth = 60.0f;
constexpr float kDragMarkHeight = 5.0f;
constexpr float kDragMarkGap = 8.0f;
constexpr float kSlotGapY = 26.0f;
constexpr float kSelectionFrameInset = 10.0f;
constexpr float kInputPortWidth = 28.0f;
constexpr float kInputPortHeight = 52.0f;
constexpr float kMinimumWidth = view::CacheLineView::kWidth + kHorizontalPadding * 2.0f;
constexpr float kMinimumHeight = view::CacheLineView::kHeight + kSlotsOffsetY + kBottomPadding;
constexpr unsigned int kTitleTextSize = 30;
constexpr unsigned int kSummaryTextSize = 18;

const sf::Color kContainerFillColor(36, 44, 60);
const sf::Color kContainerOutlineColor(88, 112, 150);
const sf::Color kInputPortFillColor(48, 58, 78);
const sf::Color kInputPortOutlineColor(132, 154, 191);
const sf::Color kSelectionOutlineColor(219, 239, 255);
const sf::Color kEmptyOverlayColor(12, 20, 34, 155);
const sf::Color kSelectedEmptyOverlayColor(71, 100, 142, 100);
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

sf::Vector2f computeCacheSize(std::size_t slotCount) {
    if (slotCount == 0) {
        return {kMinimumWidth, kMinimumHeight};
    }

    const float slotsHeight = static_cast<float>(slotCount) * view::CacheLineView::kHeight +
                              static_cast<float>(slotCount - 1) * kSlotGapY;

    return {kMinimumWidth, kSlotsOffsetY + slotsHeight + kBottomPadding};
}
} // namespace

namespace view {
CacheView::CacheView(const sf::Font* font, sf::Vector2f position) : m_font(font), m_position(position) {
    m_container.setFillColor(kContainerFillColor);

    m_selectionFrame.setFillColor(sf::Color::Transparent);

    rebuildContainer();
    rebuildText();
    layout();
}

void CacheView::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

sf::FloatRect CacheView::getBounds() const {
    return {m_position, m_size};
}

bool CacheView::isInDragHandle(sf::Vector2f worldPoint) const {
    const sf::FloatRect dragHandleBounds{m_position, {m_size.x, kDragHandleHeight}};
    return dragHandleBounds.contains(worldPoint);
}

void CacheView::setDragState(bool hovered, bool dragging) {
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

sf::Vector2f CacheView::getLinePosition() const {
    if (m_slotViews.empty()) {
        return m_position;
    }

    const std::size_t selectedIndex = std::min(m_selectedSlotIndex, m_slotViews.size() - 1);
    return m_slotViews[selectedIndex].getPosition();
}

sf::Vector2f CacheView::getEntryPosition() const {
    const sf::FloatRect portBounds = m_inputPort.getGlobalBounds();
    const sf::Vector2f portCenter{portBounds.position.x + portBounds.size.x * 0.5f,
                                  portBounds.position.y + portBounds.size.y * 0.5f};
    return {portCenter.x - CacheLineView::kWidth * 0.5f, portCenter.y - CacheLineView::kHeight * 0.5f};
}

void CacheView::setFont(const sf::Font* font) {
    m_font = font;

    for (CacheLineView& slotView : m_slotViews) {
        slotView.setFont(font);
    }

    rebuildText();
    layout();
}

void CacheView::sync(const sim::Cache& cache, const sim::MemoryTransaction* activeTransaction) {
    m_cacheSizeInBytes = cache.getSizeInBytes();

    if (cache.getSlotCount() != m_slotViews.size()) {
        rebuildSlots(cache.getSlotCount());
    }

    if (!m_slotViews.empty()) {
        if (activeTransaction) {
            m_selectedSlotIndex = activeTransaction->getTargetCacheSlotIndex() % m_slotViews.size();
        } else if (m_selectedSlotIndex >= m_slotViews.size()) {
            m_selectedSlotIndex = 0;
        }
    } else {
        m_selectedSlotIndex = 0;
    }

    m_selectedSlotValid = false;

    for (std::size_t index = 0; index < m_slotOverlays.size(); ++index) {
        const sim::CacheSlotInfo slotInfo = cache.getSlotInfo(index);
        sf::Color overlayColor = slotInfo.valid ? sf::Color::Transparent : kEmptyOverlayColor;

        if (index == m_selectedSlotIndex) {
            m_selectedSlotValid = slotInfo.valid;

            if (!slotInfo.valid) {
                overlayColor = kSelectedEmptyOverlayColor;
            }
        }

        m_slotOverlays[index].setFillColor(overlayColor);
    }

    rebuildText();
    layout();
}

void CacheView::rebuildContainer() {
    m_size = computeCacheSize(m_slotViews.size());
    m_container.setOutlineThickness(0.0f);
    buildRoundedRect(m_container, m_size, kCornerRadius);
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(kContainerOutlineColor);

    m_inputPort.setOutlineThickness(0.0f);
    buildRoundedRect(m_inputPort, {kInputPortWidth, kInputPortHeight}, 12.0f);
    m_inputPort.setOutlineThickness(3.0f);
    m_inputPort.setFillColor(kInputPortFillColor);
    m_inputPort.setOutlineColor(kInputPortOutlineColor);

    m_dragHandleOverlay.setOutlineThickness(0.0f);
    buildRoundedRect(m_dragHandleOverlay, {m_size.x, kDragHandleHeight}, kCornerRadius);
    m_dragHandleOverlay.setFillColor(sf::Color::Transparent);

    m_dragHandleMarks.assign(3, sf::RectangleShape{{kDragMarkWidth, kDragMarkHeight}});
    for (sf::RectangleShape& mark : m_dragHandleMarks) {
        mark.setOrigin({kDragMarkWidth * 0.5f, kDragMarkHeight * 0.5f});
        mark.setFillColor(kDragMarkIdleColor);
    }
}

void CacheView::rebuildSlots(std::size_t slotCount) {
    m_slotViews.clear();
    m_slotOverlays.clear();
    m_slotViews.reserve(slotCount);
    m_slotOverlays.reserve(slotCount);

    for (std::size_t index = 0; index < slotCount; ++index) {
        m_slotViews.emplace_back(m_font);

        sf::ConvexShape overlay;
        buildRoundedRect(overlay, {CacheLineView::kWidth, CacheLineView::kHeight}, 16.0f);
        overlay.setFillColor(kEmptyOverlayColor);
        m_slotOverlays.push_back(overlay);
    }

    rebuildContainer();
    rebuildText();
    layout();
}

void CacheView::rebuildText() {
    m_titleText.reset();
    m_summaryText.reset();

    if (!m_font) {
        return;
    }

    m_titleText.emplace(*m_font, "Cache", kTitleTextSize);
    m_titleText->setFillColor(sf::Color(233, 245, 255));

    if (m_cacheSizeInBytes > 0) {
        const std::string summary =
            std::to_string(m_slotViews.size()) + " slots  |  " + std::to_string(m_cacheSizeInBytes) + " B";
        m_summaryText.emplace(*m_font, summary, kSummaryTextSize);
        m_summaryText->setFillColor(sf::Color(182, 203, 229));
    }
}

void CacheView::layout() {
    m_container.setPosition(m_position);
    m_dragHandleOverlay.setPosition(m_position);

    const float marksCenterX = m_position.x + m_size.x * 0.5f;
    const float marksStartY = m_position.y + 22.0f;
    for (std::size_t index = 0; index < m_dragHandleMarks.size(); ++index) {
        m_dragHandleMarks[index].setPosition(
            {marksCenterX, marksStartY + static_cast<float>(index) * (kDragMarkHeight + kDragMarkGap)});
    }

    if (m_titleText) {
        m_titleText->setPosition({m_position.x + kTitleOffsetX, m_position.y + kTitleOffsetY});
    }

    if (m_summaryText) {
        m_summaryText->setPosition({m_position.x + kTitleOffsetX, m_position.y + kSummaryOffsetY});
    }

    const float slotX = m_position.x + kHorizontalPadding;
    const float selectionSizeX = CacheLineView::kWidth + kSelectionFrameInset * 2.0f;
    const float selectionSizeY = CacheLineView::kHeight + kSelectionFrameInset * 2.0f;

    m_selectionFrame.setOutlineThickness(0.0f);
    buildRoundedRect(m_selectionFrame, {selectionSizeX, selectionSizeY}, 20.0f);
    m_selectionFrame.setFillColor(sf::Color::Transparent);
    m_selectionFrame.setOutlineThickness(4.0f);
    m_selectionFrame.setOutlineColor(kSelectionOutlineColor);

    for (std::size_t index = 0; index < m_slotViews.size(); ++index) {
        const float slotY =
            m_position.y + kSlotsOffsetY + static_cast<float>(index) * (CacheLineView::kHeight + kSlotGapY);
        m_slotViews[index].setPosition({slotX, slotY});
        m_slotOverlays[index].setPosition({slotX, slotY});

        if (index == m_selectedSlotIndex) {
            m_selectionFrame.setPosition({slotX - kSelectionFrameInset, slotY - kSelectionFrameInset});
        }
    }

    if (!m_slotViews.empty()) {
        const std::size_t selectedIndex = std::min(m_selectedSlotIndex, m_slotViews.size() - 1);
        const sf::Vector2f selectedLinePosition = m_slotViews[selectedIndex].getPosition();
        m_inputPort.setPosition(
            {m_position.x + m_size.x - kInputPortWidth * 0.5f,
             selectedLinePosition.y + CacheLineView::kHeight * 0.5f - kInputPortHeight * 0.5f});
    } else {
        m_inputPort.setPosition(
            {m_position.x + m_size.x - kInputPortWidth * 0.5f,
             m_position.y + m_size.y * 0.5f - kInputPortHeight * 0.5f});
    }
}

void CacheView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);
    target.draw(m_inputPort, states);
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

    for (std::size_t index = 0; index < m_slotViews.size(); ++index) {
        if (index == m_selectedSlotIndex) {
            target.draw(m_selectionFrame, states);
        }

        target.draw(m_slotViews[index], states);
        target.draw(m_slotOverlays[index], states);
    }
}
} // namespace view
