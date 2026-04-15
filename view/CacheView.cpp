#include "view/CacheView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr float kHorizontalPadding = 34.0f;
constexpr float kTopPadding = 28.0f;
constexpr float kBottomPadding = 28.0f;
constexpr float kTitleOffsetX = 24.0f;
constexpr float kTitleOffsetY = 12.0f;
constexpr float kSummaryOffsetY = 54.0f;
constexpr float kSlotsOffsetY = 94.0f;
constexpr float kSlotGapY = 26.0f;
constexpr float kSelectionFrameInset = 10.0f;
constexpr float kMinimumWidth = view::CacheLineView::kWidth + kHorizontalPadding * 2.0f;
constexpr float kMinimumHeight = view::CacheLineView::kHeight + kSlotsOffsetY + kBottomPadding;
constexpr unsigned int kTitleTextSize = 34;
constexpr unsigned int kSummaryTextSize = 18;

const sf::Color kContainerFillColor(31, 62, 101);
const sf::Color kContainerOutlineColor(22, 51, 87);
const sf::Color kSelectionOutlineColor(219, 239, 255);
const sf::Color kEmptyOverlayColor(12, 20, 34, 155);
const sf::Color kSelectedEmptyOverlayColor(71, 100, 142, 100);

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

sf::Vector2f CacheView::getLinePosition() const {
    if (m_slotViews.empty()) {
        return m_position;
    }

    const std::size_t selectedIndex = std::min(m_selectedSlotIndex, m_slotViews.size() - 1);
    return m_slotViews[selectedIndex].getPosition();
}

sf::Vector2f CacheView::getEntryPosition() const {
    if (m_slotViews.empty()) {
        return m_position;
    }

    const std::size_t selectedIndex = std::min(m_selectedSlotIndex, m_slotViews.size() - 1);
    return m_slotViews[selectedIndex].getEntryPosition();
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
    m_container.setOutlineThickness(10.0f);
    m_container.setOutlineColor(kContainerOutlineColor);
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
}

void CacheView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);

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
