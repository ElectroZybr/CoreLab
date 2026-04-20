#include "view/CacheView.h"

#include <cmath>
#include <limits>
#include <numbers>

#include "view/rails/RailBuilder.h"

namespace {
constexpr float kLeftPadding = 34.0f;
constexpr float kBottomPadding = 28.0f;
constexpr float kSlotsOffsetY = 138.0f;
constexpr float kDragHandleHeight = kSlotsOffsetY - 16.0f;
constexpr float kSlotGapY = 26.0f;
constexpr float kInputPortWidth = 52.0f;
constexpr float kInputPortHeight = 28.0f;
constexpr float kTrackThickness = 6.0f;
constexpr float kCollectorTurnRadius = 100.0f;
constexpr float kCollectorCenterInsetX = 128.0f;
constexpr float kRightPadding = kCollectorCenterInsetX + kCollectorTurnRadius + 36.0f;
constexpr float kMinimumWidth = view::CacheLineView::kWidth + kLeftPadding + kRightPadding;
constexpr float kMinimumHeight = view::CacheLineView::kHeight + kSlotsOffsetY + kBottomPadding;

const sf::Color kInputPortFillColor(48, 58, 78);
const sf::Color kInputPortOutlineColor(132, 154, 191);
const sf::Color kHighlightedInputPortFillColor(247, 214, 92, 90);
const sf::Color kHighlightedInputPortOutlineColor(247, 214, 92, 220);
const sf::Color kTrackColor(116, 134, 165);
const sf::Color kHighlightTrackColor(247, 214, 92, 210);
const sf::Color kEmptyOverlayColor(12, 20, 34, 155);
const sf::Color kSelectedEmptyOverlayColor(71, 100, 142, 100);
const sf::Color kTransparentPortColor(0, 0, 0, 0);

sf::Vector2f computeCacheSize(std::size_t slotCount) {
    if (slotCount == 0) {
        return {kMinimumWidth, kMinimumHeight};
    }

    const float slotsHeight = static_cast<float>(slotCount) * view::CacheLineView::kHeight +
                              static_cast<float>(slotCount - 1) * kSlotGapY;

    return {std::max(kMinimumWidth, view::CacheLineView::kWidth + kLeftPadding + kRightPadding),
            kSlotsOffsetY + slotsHeight + kBottomPadding};
}

float computeSlotCenterY(sf::Vector2f position, std::size_t row) {
    return position.y + kSlotsOffsetY + static_cast<float>(row) * (view::CacheLineView::kHeight + kSlotGapY) +
           view::CacheLineView::kHeight * 0.5f;
}
} // namespace

namespace view {
CacheView::CacheView(const sf::Font* font, sf::Vector2f position)
    : BlockView(font, position, computeCacheSize(0)) {
    setHeaderLayout({kDragHandleHeight, 18.0f, 98.0f, 74, 18});
    setTitle("Cache");
    addPort("mem_in", PortKind::Input, PortDirection::Down, PayloadKind::CacheLine);
    addPort("cpu_out", PortKind::Output, PortDirection::Left, PayloadKind::CacheLine);
    rebuildContainer();
    layoutBlock();
}

void CacheView::setHighlightedSlot(std::optional<std::size_t> slotIndex) {
    m_highlightedSlotIndex = slotIndex;
    layoutBlock();
}

sf::Vector2f CacheView::getLinePosition() const {
    if (m_slotViews.empty()) {
        return getWorldPosition();
    }

    const std::size_t selectedIndex = std::min(m_selectedSlotIndex, m_slotViews.size() - 1);
    return m_slotViews[selectedIndex].getPosition();
}

sf::Vector2f CacheView::getLinePosition(std::size_t slotIndex) const {
    if (m_slotViews.empty()) {
        return getWorldPosition();
    }

    return m_slotViews[std::min(slotIndex, m_slotViews.size() - 1)].getPosition();
}

sf::Vector2f CacheView::getEntryPosition() const {
    const sf::FloatRect portBounds = m_inputPort.getGlobalBounds();
    const sf::Vector2f portCenter{portBounds.position.x + portBounds.size.x * 0.5f,
                                  portBounds.position.y + portBounds.size.y * 0.5f};
    return {portCenter.x - CacheLineView::kWidth * 0.5f, portCenter.y - CacheLineView::kHeight * 0.5f};
}

sf::Vector2f CacheView::getLineHeadCenter() const {
    if (m_slotViews.empty()) {
        return getWorldPosition();
    }

    return getLineHeadCenter(m_selectedSlotIndex);
}

sf::Vector2f CacheView::getLineHeadCenter(std::size_t slotIndex) const {
    if (m_slotViews.empty()) {
        return getWorldPosition();
    }

    const sf::Vector2f topLeft = m_slotViews[std::min(slotIndex, m_slotViews.size() - 1)].getPosition();
    return {topLeft.x, topLeft.y + CacheLineView::kHeight * 0.5f};
}

sf::Vector2f CacheView::getEntryCenter() const {
    const sf::FloatRect portBounds = m_inputPort.getGlobalBounds();
    return {portBounds.position.x + portBounds.size.x * 0.5f, portBounds.position.y + portBounds.size.y * 0.5f};
}

void CacheView::sync(const sim::Cache& cache, const sim::RAM* ram, const sim::MemoryTransaction* activeTransaction) {
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

        if (slotInfo.valid && ram) {
            const std::size_t lineIndex =
                static_cast<std::size_t>(slotInfo.baseAddress / sim::RAM::kCacheLineSizeInBytes);
            if (lineIndex < ram->getLineCount()) {
                m_slotViews[index].setCellLabels(ram->getLineCellLabels(lineIndex));
            }
        } else {
            sim::RAM::LineCellLabels emptyLabels{};
            emptyLabels.fill("");
            m_slotViews[index].setCellLabels(emptyLabels);
        }

        if (index == m_selectedSlotIndex) {
            m_selectedSlotValid = slotInfo.valid;
            if (!slotInfo.valid) {
                overlayColor = kSelectedEmptyOverlayColor;
            }
        }

        m_slotOverlays[index].setFillColor(overlayColor);
    }

    setSubtitle(std::to_string(m_slotViews.size()) + " slots  |  " + std::to_string(m_cacheSizeInBytes) + " B");
    layoutBlock();
}

void CacheView::rebuildContainer() {
    setBlockSize(computeCacheSize(m_slotViews.size()));

    m_inputPort.setOutlineThickness(0.0f);
    BlockView::buildRoundedRect(m_inputPort, {kInputPortWidth, kInputPortHeight}, 12.0f);
    m_inputPort.setOutlineThickness(3.0f);
    m_inputPort.setFillColor(kInputPortFillColor);
    m_inputPort.setOutlineColor(kInputPortOutlineColor);
}

void CacheView::rebuildSlots(std::size_t slotCount) {
    m_slotViews.clear();
    m_slotOverlays.clear();
    m_installPaths.clear();
    m_slotViews.reserve(slotCount);
    m_slotOverlays.reserve(slotCount);

    for (std::size_t index = 0; index < slotCount; ++index) {
        m_slotViews.emplace_back(getFont());

        sf::ConvexShape overlay;
        BlockView::buildRoundedRect(overlay, {CacheLineView::kWidth, CacheLineView::kHeight}, 16.0f);
        overlay.setFillColor(kEmptyOverlayColor);
        m_slotOverlays.push_back(overlay);
    }

    rebuildContainer();
    layoutBlock();
}

void CacheView::layoutBlock() {
    BlockView::layoutBlock();

    const sf::Vector2f position = getWorldPosition();
    const sf::Vector2f size = getBlockSize();
    m_railPaths.clear();
    m_installPaths.clear();
    m_highlightPath = rails::RailPath({kTrackThickness + 1.0f, kHighlightTrackColor});

    const float slotX = position.x + kLeftPadding;
    const rails::RailStyle railStyle{kTrackThickness, kTrackColor};
    const float collectorCenterX = position.x + size.x - kCollectorCenterInsetX;

    for (std::size_t index = 0; index < m_slotViews.size(); ++index) {
        const float slotY =
            position.y + kSlotsOffsetY + static_cast<float>(index) * (CacheLineView::kHeight + kSlotGapY);
        m_slotViews[index].setPosition({slotX, slotY});
        m_slotOverlays[index].setPosition({slotX, slotY});
    }

    const float portCenterY = position.y;
    const float portCenterX = collectorCenterX;
    m_inputPort.setPosition(
        {collectorCenterX - kInputPortWidth * 0.5f, portCenterY - kInputPortHeight * 0.5f});
    if (m_highlightedSlotIndex.has_value()) {
        m_inputPort.setFillColor(kHighlightedInputPortFillColor);
        m_inputPort.setOutlineColor(kHighlightedInputPortOutlineColor);
    } else {
        m_inputPort.setFillColor(kInputPortFillColor);
        m_inputPort.setOutlineColor(kInputPortOutlineColor);
    }

    if (PortView* inputPort = findPort("mem_in")) {
        inputPort->setLocalAnchor({portCenterX - position.x, 0.0f});
        inputPort->setSize({kInputPortWidth, kInputPortHeight});
        inputPort->setColors(kTransparentPortColor, kTransparentPortColor);
    }

    if (PortView* outputPort = findPort("cpu_out")) {
        outputPort->setLocalAnchor({0.0f, size.y * 0.5f});
        outputPort->setSize({kInputPortWidth, kInputPortHeight});
        outputPort->setColors(kTransparentPortColor, kTransparentPortColor);
    }

    if (m_slotViews.empty()) {
        return;
    }

    m_installPaths.reserve(m_slotViews.size());

    float collectorMinY = std::numeric_limits<float>::max();
    float collectorMaxY = -std::numeric_limits<float>::max();

    for (std::size_t index = 0; index < m_slotViews.size(); ++index) {
        const float slotCenterY = computeSlotCenterY(position, index);
        const float slotRightX = slotX + CacheLineView::kWidth;
        const float lineEntryCenterX = slotX;

        rails::RailPath installPath(railStyle);
        rails::RailPath highlightInstallPath({kTrackThickness + 1.0f, kHighlightTrackColor});

        if (slotCenterY < portCenterY + kCollectorTurnRadius + 0.5f) {
            m_railPaths.push_back(rails::RailBuilder::straight(
                sf::Vector2f{slotRightX, slotCenterY}, sf::Vector2f{collectorCenterX, slotCenterY}, railStyle));
            collectorMinY = std::min(collectorMinY, slotCenterY);
            collectorMaxY = std::max(collectorMaxY, slotCenterY);

            if (portCenterY < slotCenterY - 0.5f) {
                installPath.appendStraight({collectorCenterX, portCenterY}, {collectorCenterX, slotCenterY});
                highlightInstallPath.appendStraight({collectorCenterX, portCenterY},
                                                   {collectorCenterX, slotCenterY});
            }
            if (lineEntryCenterX < collectorCenterX - 0.5f) {
                installPath.appendStraight({collectorCenterX, slotCenterY}, {lineEntryCenterX, slotCenterY});
                highlightInstallPath.appendStraight({collectorCenterX, slotCenterY},
                                                   {lineEntryCenterX, slotCenterY});
            }
        } else {
            m_railPaths.push_back(rails::RailBuilder::straight(
                sf::Vector2f{slotRightX, slotCenterY},
                sf::Vector2f{collectorCenterX - kCollectorTurnRadius, slotCenterY},
                railStyle));

            rails::RailPath rowTurn(railStyle);
            rowTurn.appendArc(sf::Vector2f{collectorCenterX - kCollectorTurnRadius,
                                           slotCenterY - kCollectorTurnRadius},
                              kCollectorTurnRadius,
                              std::numbers::pi_v<float> * 0.5f,
                              0.0f);
            m_railPaths.push_back(std::move(rowTurn));

            collectorMinY = std::min(collectorMinY, slotCenterY - kCollectorTurnRadius);
            collectorMaxY = std::max(collectorMaxY, slotCenterY - kCollectorTurnRadius);

            const float turnStartY = slotCenterY - kCollectorTurnRadius;
            if (portCenterY < turnStartY - 0.5f) {
                installPath.appendStraight({collectorCenterX, portCenterY}, {collectorCenterX, turnStartY});
                highlightInstallPath.appendStraight({collectorCenterX, portCenterY},
                                                   {collectorCenterX, turnStartY});
            }
            installPath.appendArc({collectorCenterX - kCollectorTurnRadius, slotCenterY - kCollectorTurnRadius},
                                  kCollectorTurnRadius,
                                  0.0f,
                                  std::numbers::pi_v<float> * 0.5f);
            highlightInstallPath.appendArc({collectorCenterX - kCollectorTurnRadius,
                                            slotCenterY - kCollectorTurnRadius},
                                           kCollectorTurnRadius,
                                           0.0f,
                                           std::numbers::pi_v<float> * 0.5f);
            if (lineEntryCenterX < collectorCenterX - kCollectorTurnRadius - 0.5f) {
                installPath.appendStraight({collectorCenterX - kCollectorTurnRadius, slotCenterY},
                                          {lineEntryCenterX, slotCenterY});
                highlightInstallPath.appendStraight({collectorCenterX - kCollectorTurnRadius, slotCenterY},
                                                   {lineEntryCenterX, slotCenterY});
            }
        }

        m_installPaths.push_back(std::move(installPath));
        if (m_highlightedSlotIndex && *m_highlightedSlotIndex == index) {
            m_highlightPath = std::move(highlightInstallPath);
        }
    }

    collectorMinY = std::min(collectorMinY, portCenterY);
    if (collectorMinY < collectorMaxY - 0.5f) {
        m_railPaths.push_back(rails::RailBuilder::straight(sf::Vector2f{collectorCenterX, collectorMinY},
                                                           sf::Vector2f{collectorCenterX, collectorMaxY},
                                                           railStyle));
    }
}

void CacheView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_inputPort, states);

    for (const rails::RailPath& path : m_railPaths) {
        target.draw(path, states);
    }

    if (!m_highlightPath.isEmpty()) {
        target.draw(m_highlightPath, states);
    }

    for (std::size_t index = 0; index < m_slotViews.size(); ++index) {
        target.draw(m_slotViews[index], states);
        target.draw(m_slotOverlays[index], states);
    }
}
} // namespace view
