#include "view/RamView.h"

#include "sim/Math.h"
#include "view/rails/RailBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>
#include <string>

namespace {
constexpr sf::Vector2f kSlotSize{view::CacheLineView::kWidth, view::CacheLineView::kHeight};
constexpr float kColumnGap = 20.0f;
constexpr float kRowGap = 110.0f;
constexpr float kCollectorCenterInsetX = 128.0f;
constexpr float kRightPadding = 24.0f;
constexpr float kBottomPadding = 24.0f;
constexpr float kSlotsTopOffset = 138.0f;
constexpr float kDragHandleHeight = kSlotsTopOffset - 16.0f;
constexpr float kMinimumWidth = 320.0f;
constexpr std::size_t kMaxColumns = 6;
constexpr float kTrackThickness = 6.0f;
constexpr float kCollectorTurnRadius = 100.0f;
constexpr float kLeftPadding = kCollectorCenterInsetX + kCollectorTurnRadius + 24.0f;
constexpr float kOutputPortWidth = 28.0f;
constexpr float kOutputPortHeight = 52.0f;
const sf::Color kTrackColor(116, 134, 165);
const sf::Color kHighlightTrackColor(247, 214, 92, 210);
const sf::Color kOutputPortFillColor(48, 58, 78);
const sf::Color kOutputPortOutlineColor(132, 154, 191);
const sf::Color kTransparentPortColor(0, 0, 0, 0);

struct TangentBridge {
    bool valid = false;
    std::size_t row = 0;
    sf::Vector2f rowPoint{0.0f, 0.0f};
    float rowAngle = 0.0f;
    sf::Vector2f outputPoint{0.0f, 0.0f};
    float outputAngle = 0.0f;
};

sf::Vector2f computeRamSize(std::size_t slotCount) {
    if (slotCount == 0) {
        return {kMinimumWidth, kSlotsTopOffset + kBottomPadding};
    }

    const std::size_t columns = std::min(slotCount, kMaxColumns);
    const std::size_t rows = math::ceilDiv(slotCount, columns);

    const float slotsWidth =
        static_cast<float>(columns) * kSlotSize.x + static_cast<float>(columns - 1) * kColumnGap;
    const float slotsHeight = static_cast<float>(rows) * kSlotSize.y + static_cast<float>(rows) * kRowGap;

    return {std::max(kMinimumWidth, kLeftPadding + slotsWidth + kRightPadding),
            kSlotsTopOffset + slotsHeight + kBottomPadding};
}

float computeLaneTop(float rowY) {
    return rowY + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f;
}

float computeLaneCenterY(sf::Vector2f position, std::size_t row) {
    const float rowY = position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
    return computeLaneTop(rowY) + kSlotSize.y * 0.5f;
}

bool isAngleInsideArc(float angle, float startAngle, float endAngle) {
    const float minAngle = std::min(startAngle, endAngle) - 0.001f;
    const float maxAngle = std::max(startAngle, endAngle) + 0.001f;
    return angle >= minAngle && angle <= maxAngle;
}

std::optional<TangentBridge>
findNearestUpperTangent(sf::Vector2f position, std::size_t rows, float busCenterX, float junctionCenterY) {
    float bestDistance = std::numeric_limits<float>::max();
    std::optional<std::size_t> bestRow;

    for (std::size_t row = 0; row < rows; ++row) {
        const float laneCenterY = computeLaneCenterY(position, row);
        const float distanceToCenter = junctionCenterY - laneCenterY;
        if (distanceToCenter > 0.5f && distanceToCenter < bestDistance) {
            bestDistance = distanceToCenter;
            bestRow = row;
        }
    }

    if (!bestRow) {
        return std::nullopt;
    }

    const float laneCenterY = computeLaneCenterY(position, *bestRow);
    const sf::Vector2f rowCenter{busCenterX + kCollectorTurnRadius, laneCenterY + kCollectorTurnRadius};
    const sf::Vector2f outputCenter{busCenterX - kCollectorTurnRadius,
                                    junctionCenterY - kCollectorTurnRadius};
    const sf::Vector2f delta = outputCenter - rowCenter;
    const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
    const float radiusDelta = kCollectorTurnRadius - (-kCollectorTurnRadius);
    const float tangentSquared = distanceSquared - radiusDelta * radiusDelta;
    if (tangentSquared <= 0.0f) {
        return std::nullopt;
    }

    const sf::Vector2f perpendicular{-delta.y, delta.x};

    for (float sign : {1.0f, -1.0f}) {
        const sf::Vector2f normal =
            (delta * radiusDelta + perpendicular * std::sqrt(tangentSquared) * sign) / distanceSquared;
        const sf::Vector2f rowPoint = rowCenter + normal * kCollectorTurnRadius;
        const sf::Vector2f outputPoint = outputCenter - normal * kCollectorTurnRadius;
        const float rowAngle = std::atan2(normal.y, normal.x);
        const float outputAngle = std::atan2(-normal.y, -normal.x);

        if (isAngleInsideArc(rowAngle, -std::numbers::pi_v<float>, -std::numbers::pi_v<float> * 0.5f) &&
            isAngleInsideArc(outputAngle, 0.0f, std::numbers::pi_v<float> * 0.5f) &&
            outputPoint.x < rowPoint.x && outputPoint.y > rowPoint.y) {
            return TangentBridge{true, *bestRow, rowPoint, rowAngle, outputPoint, outputAngle};
        }
    }

    return std::nullopt;
}

std::optional<TangentBridge>
findNearestLowerTangent(sf::Vector2f position, std::size_t rows, float busCenterX, float junctionCenterY) {
    float bestDistance = std::numeric_limits<float>::max();
    std::optional<std::size_t> bestRow;

    for (std::size_t row = 0; row < rows; ++row) {
        const float laneCenterY = computeLaneCenterY(position, row);
        const float distanceToCenter = laneCenterY - junctionCenterY;
        if (distanceToCenter > 0.5f && distanceToCenter < bestDistance) {
            bestDistance = distanceToCenter;
            bestRow = row;
        }
    }

    if (!bestRow) {
        return std::nullopt;
    }

    const float laneCenterY = computeLaneCenterY(position, *bestRow);
    const sf::Vector2f rowCenter{busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius};
    const sf::Vector2f outputCenter{busCenterX - kCollectorTurnRadius,
                                    junctionCenterY + kCollectorTurnRadius};
    const sf::Vector2f delta = outputCenter - rowCenter;
    const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
    const float radiusDelta = kCollectorTurnRadius - (-kCollectorTurnRadius);
    const float tangentSquared = distanceSquared - radiusDelta * radiusDelta;
    if (tangentSquared <= 0.0f) {
        return std::nullopt;
    }

    const sf::Vector2f perpendicular{-delta.y, delta.x};

    for (float sign : {1.0f, -1.0f}) {
        const sf::Vector2f normal =
            (delta * radiusDelta + perpendicular * std::sqrt(tangentSquared) * sign) / distanceSquared;
        const sf::Vector2f rowPoint = rowCenter + normal * kCollectorTurnRadius;
        const sf::Vector2f outputPoint = outputCenter - normal * kCollectorTurnRadius;
        const float rowAngle = std::atan2(normal.y, normal.x);
        const float outputAngle = std::atan2(-normal.y, -normal.x);

        if (isAngleInsideArc(rowAngle, std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float>) &&
            isAngleInsideArc(outputAngle, -std::numbers::pi_v<float> * 0.5f, 0.0f) &&
            outputPoint.x < rowPoint.x && outputPoint.y < rowPoint.y) {
            return TangentBridge{true, *bestRow, rowPoint, rowAngle, outputPoint, outputAngle};
        }
    }

    return std::nullopt;
}
} // namespace

namespace view {
RamView::RamView(std::size_t sizeInBytes, const sf::Font* font, sf::Vector2f position)
    : BlockView(font, position, computeRamSize(math::ceilDiv(sizeInBytes, kCacheLineSizeInBytes))),
      m_sizeInBytes(sizeInBytes) {
    m_slotCount = math::ceilDiv(m_sizeInBytes, kCacheLineSizeInBytes);
    setHeaderLayout({kDragHandleHeight, 18.0f, 98.0f, 74, 18});
    setTitle("RAM");
    setSubtitle(std::to_string(m_sizeInBytes) + " B");
    addPort("mem_out", PortKind::Output, PortDirection::Left, PayloadKind::CacheLine);
    rebuildGeometry();
    layoutBlock();
}

void RamView::setHighlightedLine(std::optional<std::size_t> lineIndex) {
    m_highlightedLineIndex = lineIndex;
    layoutBlock();
}

sf::Vector2f RamView::getLinePosition(std::size_t index) const {
    if (index >= m_lines.size()) {
        return getPosition();
    }

    return m_lines[index].getPosition();
}

sf::Vector2f RamView::getLineHeadCenter(std::size_t index) const {
    if (index >= m_lines.size()) {
        return getPosition();
    }

    const sf::Vector2f topLeft = m_lines[index].getPosition();
    return {topLeft.x, topLeft.y + CacheLineView::kHeight * 0.5f};
}

sim::RAM::LineCellLabels RamView::getLineLabels(std::size_t index) const {
    sim::RAM::LineCellLabels labels{};
    labels.fill("");

    if (index >= m_lines.size()) {
        return labels;
    }

    return labels;
}

void RamView::sync(const sim::RAM& ram) {
    const std::size_t lineCount = std::min(m_lines.size(), ram.getLineCount());
    for (std::size_t index = 0; index < lineCount; ++index) {
        m_lines[index].setCellLabels(ram.getLineCellLabels(index));
    }
}

RamView::ReadPath RamView::getReadPath(std::size_t index) const {
    if (index >= m_lines.size()) {
        return {getPosition(),
                getPosition(),
                getPosition(),
                getPosition(),
                0.0f,
                0.0f,
                0.0f,
                getPosition(),
                getPosition(),
                getPosition(),
                0.0f,
                0.0f,
                0.0f,
                getPosition(),
                getPosition()};
    }

    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    const std::size_t rows = math::ceilDiv(m_slotCount, columns);
    const std::size_t row = index / columns;
    const sf::Vector2f lineTopLeft = m_lines[index].getPosition();
    const sf::Vector2f sourcePosition{lineTopLeft.x, lineTopLeft.y + CacheLineView::kHeight * 0.5f};
    const sf::Vector2f lanePosition{sourcePosition.x, computeLaneCenterY(getPosition(), row)};
    const float laneCenterY = lanePosition.y;
    const float busCenterX = getPosition().x + kCollectorCenterInsetX;
    const float outputCenterX = getPosition().x;
    const float firstLaneCenterY =
        getPosition().y + kSlotsTopOffset + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float lastLaneCenterY = getPosition().y + kSlotsTopOffset +
                                  static_cast<float>(rows - 1) * (kSlotSize.y + kRowGap) + kSlotSize.y +
                                  (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float junctionCenterY = (firstLaneCenterY + lastLaneCenterY) * 0.5f;
    const std::optional<TangentBridge> upperTangent =
        findNearestUpperTangent(getPosition(), rows, busCenterX, junctionCenterY);
    const std::optional<TangentBridge> lowerTangent =
        findNearestLowerTangent(getPosition(), rows, busCenterX, junctionCenterY);
    sf::Vector2f turnEntryPosition{busCenterX + kCollectorTurnRadius, laneCenterY};
    sf::Vector2f turnCenter{busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius};
    float turnStartAngle = std::numbers::pi_v<float> * 0.5f;
    float turnEndAngle = std::numbers::pi_v<float>;
    sf::Vector2f turnExitPosition{busCenterX, laneCenterY - kCollectorTurnRadius};
    sf::Vector2f collectorPosition{busCenterX, junctionCenterY + kCollectorTurnRadius};
    sf::Vector2f junctionTurnCenter{busCenterX - kCollectorTurnRadius, junctionCenterY + kCollectorTurnRadius};
    float junctionTurnStartAngle = 0.0f;
    float junctionTurnEndAngle = -std::numbers::pi_v<float> * 0.5f;
    sf::Vector2f junctionTurnExitPosition{busCenterX - kCollectorTurnRadius, junctionCenterY};

    if (laneCenterY < junctionCenterY - 0.5f) {
        const bool isNearestUpper = upperTangent && upperTangent->row == row;

        turnCenter = {busCenterX + kCollectorTurnRadius, laneCenterY + kCollectorTurnRadius};
        turnStartAngle = -std::numbers::pi_v<float> * 0.5f;
        turnEndAngle = isNearestUpper ? upperTangent->rowAngle : -std::numbers::pi_v<float>;
        turnExitPosition = isNearestUpper
                               ? upperTangent->rowPoint
                               : sf::Vector2f{busCenterX, laneCenterY + kCollectorTurnRadius};
        collectorPosition = isNearestUpper ? upperTangent->outputPoint
                                           : sf::Vector2f{busCenterX, junctionCenterY - kCollectorTurnRadius};
        junctionTurnCenter = {busCenterX - kCollectorTurnRadius, junctionCenterY - kCollectorTurnRadius};
        junctionTurnStartAngle = isNearestUpper ? upperTangent->outputAngle : 0.0f;
        junctionTurnEndAngle = std::numbers::pi_v<float> * 0.5f;
        junctionTurnExitPosition = {busCenterX - kCollectorTurnRadius, junctionCenterY};
    } else if (laneCenterY > junctionCenterY + 0.5f) {
        const bool isNearestLower = lowerTangent && lowerTangent->row == row;

        turnCenter = {busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius};
        turnStartAngle = std::numbers::pi_v<float> * 0.5f;
        turnEndAngle = isNearestLower ? lowerTangent->rowAngle : std::numbers::pi_v<float>;
        turnExitPosition = isNearestLower
                               ? lowerTangent->rowPoint
                               : sf::Vector2f{busCenterX, laneCenterY - kCollectorTurnRadius};
        collectorPosition = isNearestLower ? lowerTangent->outputPoint
                                           : sf::Vector2f{busCenterX, junctionCenterY + kCollectorTurnRadius};
        junctionTurnCenter = {busCenterX - kCollectorTurnRadius, junctionCenterY + kCollectorTurnRadius};
        junctionTurnStartAngle = isNearestLower ? lowerTangent->outputAngle : 0.0f;
        junctionTurnEndAngle = -std::numbers::pi_v<float> * 0.5f;
        junctionTurnExitPosition = {busCenterX - kCollectorTurnRadius, junctionCenterY};
    } else {
        turnEntryPosition = {busCenterX, laneCenterY};
        turnCenter = {busCenterX, laneCenterY};
        turnStartAngle = 0.0f;
        turnEndAngle = 0.0f;
        turnExitPosition = turnEntryPosition;
        collectorPosition = turnEntryPosition;
        junctionTurnCenter = {busCenterX, junctionCenterY};
        junctionTurnStartAngle = 0.0f;
        junctionTurnEndAngle = 0.0f;
        junctionTurnExitPosition = {busCenterX - kCollectorTurnRadius, junctionCenterY};
    }

    const sf::Vector2f exitPosition{outputCenterX, junctionCenterY};

    return {sourcePosition,
            lanePosition,
            turnEntryPosition,
            turnCenter,
            kCollectorTurnRadius,
            turnStartAngle,
            turnEndAngle,
            turnExitPosition,
            collectorPosition,
            junctionTurnCenter,
            kCollectorTurnRadius,
            junctionTurnStartAngle,
            junctionTurnEndAngle,
            junctionTurnExitPosition,
            exitPosition};
}

void RamView::rebuildGeometry() {
    setBlockSize(computeRamSize(m_slotCount));

    m_outputPort.setOutlineThickness(0.0f);
    BlockView::buildRoundedRect(m_outputPort, {kOutputPortWidth, kOutputPortHeight}, 12.0f);
    m_outputPort.setOutlineThickness(3.0f);
    m_outputPort.setFillColor(kOutputPortFillColor);
    m_outputPort.setOutlineColor(kOutputPortOutlineColor);

    m_lines.clear();
    m_lines.reserve(m_slotCount);
    for (std::size_t index = 0; index < m_slotCount; ++index) {
        m_lines.emplace_back(getFont());
    }
}

void RamView::layoutBlock() {
    BlockView::layoutBlock();

    const sf::Vector2f position = getPosition();
    const sf::Vector2f size = getBlockSize();
    m_outputPort.setPosition(
        {position.x - kOutputPortWidth * 0.5f, position.y + size.y * 0.5f - kOutputPortHeight * 0.5f});

    if (m_lines.empty()) {
        return;
    }

    m_railPaths.clear();
    m_highlightPath = rails::RailPath({kTrackThickness + 1.0f, kHighlightTrackColor});

    const rails::RailStyle railStyle{kTrackThickness, kTrackColor};
    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    const std::size_t rows = math::ceilDiv(m_slotCount, columns);
    const float busCenterX = position.x + kCollectorCenterInsetX;
    const float firstLaneCenterY =
        position.y + kSlotsTopOffset + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float lastLaneCenterY = position.y + kSlotsTopOffset +
                                  static_cast<float>(rows - 1) * (kSlotSize.y + kRowGap) + kSlotSize.y +
                                  (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float junctionCenterY = (firstLaneCenterY + lastLaneCenterY) * 0.5f;
    const float outputCenterX = position.x;
    m_outputPort.setPosition(
        {position.x - kOutputPortWidth * 0.5f, junctionCenterY - kOutputPortHeight * 0.5f});

    if (PortView* outputPort = findPort("mem_out")) {
        outputPort->setLocalAnchor({0.0f, junctionCenterY - position.y});
        outputPort->setSize({kOutputPortWidth, kOutputPortHeight});
        outputPort->setColors(kTransparentPortColor, kTransparentPortColor);
    }

    const std::optional<TangentBridge> upperTangent =
        findNearestUpperTangent(position, rows, busCenterX, junctionCenterY);
    const std::optional<TangentBridge> lowerTangent =
        findNearestLowerTangent(position, rows, busCenterX, junctionCenterY);
    float upperCollectorMinY = std::numeric_limits<float>::max();
    float lowerCollectorMaxY = -std::numeric_limits<float>::max();
    bool hasUpperRows = false;
    bool hasLowerRows = false;
    bool hasFarUpperRows = false;
    bool hasFarLowerRows = false;

    for (std::size_t row = 0; row < rows; ++row) {
        const float rowY = position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
        const float laneTopY = computeLaneTop(rowY);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;
        const float laneEndX = position.x + kLeftPadding +
                               static_cast<float>(columns - 1) * (kSlotSize.x + kColumnGap) +
                               kSlotSize.x * 0.5f;

        if (laneCenterY < junctionCenterY - 0.5f) {
            const float turnExitCenterY = laneCenterY + kCollectorTurnRadius;
            const bool isNearestUpper = upperTangent && upperTangent->row == row;

            hasUpperRows = true;

            m_railPaths.push_back(
                rails::RailBuilder::straight(sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY},
                                             sf::Vector2f{laneEndX, laneCenterY},
                                             railStyle));

            rails::RailPath upperTurn(railStyle);
            upperTurn.appendArc(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY + kCollectorTurnRadius},
                kCollectorTurnRadius,
                -std::numbers::pi_v<float> * 0.5f,
                isNearestUpper ? upperTangent->rowAngle : -std::numbers::pi_v<float>);
            m_railPaths.push_back(std::move(upperTurn));

            if (isNearestUpper) {
                m_railPaths.push_back(
                    rails::RailBuilder::straight(upperTangent->rowPoint, upperTangent->outputPoint, railStyle));
            } else {
                hasFarUpperRows = true;
                upperCollectorMinY = std::min(upperCollectorMinY, turnExitCenterY);
            }
        } else if (laneCenterY > junctionCenterY + 0.5f) {
            const float turnExitCenterY = laneCenterY - kCollectorTurnRadius;
            const bool isNearestLower = lowerTangent && lowerTangent->row == row;

            hasLowerRows = true;

            m_railPaths.push_back(
                rails::RailBuilder::straight(sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY},
                                             sf::Vector2f{laneEndX, laneCenterY},
                                             railStyle));

            rails::RailPath lowerTurn(railStyle);
            lowerTurn.appendArc(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius},
                kCollectorTurnRadius,
                std::numbers::pi_v<float> * 0.5f,
                isNearestLower ? lowerTangent->rowAngle : std::numbers::pi_v<float>);
            m_railPaths.push_back(std::move(lowerTurn));

            if (isNearestLower) {
                m_railPaths.push_back(
                    rails::RailBuilder::straight(lowerTangent->rowPoint, lowerTangent->outputPoint, railStyle));
            } else {
                hasFarLowerRows = true;
                lowerCollectorMaxY = std::max(lowerCollectorMaxY, turnExitCenterY);
            }
        } else {
            m_railPaths.push_back(
                rails::RailBuilder::straight(sf::Vector2f{busCenterX, laneCenterY},
                                             sf::Vector2f{laneEndX, laneCenterY},
                                             railStyle));
        }
    }

    if (hasUpperRows) {
        const float upperCollectorEndY = junctionCenterY - kCollectorTurnRadius;

        if (hasFarUpperRows && upperCollectorMinY < upperCollectorEndY - 0.5f) {
            m_railPaths.push_back(rails::RailBuilder::straight(sf::Vector2f{busCenterX, upperCollectorMinY},
                                                               sf::Vector2f{busCenterX, upperCollectorEndY},
                                                               railStyle));
        }

        rails::RailPath upperExit(railStyle);
        upperExit.appendArc(
            sf::Vector2f{busCenterX - kCollectorTurnRadius, junctionCenterY - kCollectorTurnRadius},
            kCollectorTurnRadius,
            0.0f,
            std::numbers::pi_v<float> * 0.5f);
        m_railPaths.push_back(std::move(upperExit));
    }

    if (hasLowerRows) {
        const float lowerCollectorStartY = junctionCenterY + kCollectorTurnRadius;

        if (hasFarLowerRows && lowerCollectorStartY < lowerCollectorMaxY - 0.5f) {
            m_railPaths.push_back(rails::RailBuilder::straight(sf::Vector2f{busCenterX, lowerCollectorStartY},
                                                               sf::Vector2f{busCenterX, lowerCollectorMaxY},
                                                               railStyle));
        }

        rails::RailPath lowerExit(railStyle);
        lowerExit.appendArc(
            sf::Vector2f{busCenterX - kCollectorTurnRadius, junctionCenterY + kCollectorTurnRadius},
            kCollectorTurnRadius,
            0.0f,
            -std::numbers::pi_v<float> * 0.5f);
        m_railPaths.push_back(std::move(lowerExit));
    }

    const float outputStartX = busCenterX - kCollectorTurnRadius;
    m_railPaths.push_back(rails::RailBuilder::straight(sf::Vector2f{outputCenterX, junctionCenterY},
                                                       sf::Vector2f{outputStartX, junctionCenterY},
                                                       railStyle));

    for (std::size_t index = 0; index < m_lines.size(); ++index) {
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;

        const float x = position.x + kLeftPadding + static_cast<float>(column) * (kSlotSize.x + kColumnGap);
        const float y = position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
        const float laneTopY = computeLaneTop(y);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;

        m_lines[index].setPosition({x, y});

        m_railPaths.push_back(
            rails::RailBuilder::straight(sf::Vector2f{x + kSlotSize.x * 0.5f, y + kSlotSize.y},
                                         sf::Vector2f{x + kSlotSize.x * 0.5f, laneCenterY},
                                         railStyle));
    }

    if (m_highlightedLineIndex && *m_highlightedLineIndex < m_lines.size()) {
        const ReadPath path = getReadPath(*m_highlightedLineIndex);
        const rails::RailStyle highlightStyle{kTrackThickness + 1.0f, kHighlightTrackColor};
        const sf::Vector2f lineTopLeft = m_lines[*m_highlightedLineIndex].getPosition();
        const sf::Vector2f railEntryPoint{
            lineTopLeft.x + kSlotSize.x * 0.5f,
            lineTopLeft.y + kSlotSize.y,
        };
        const sf::Vector2f highlightLanePoint{railEntryPoint.x, path.lanePosition.y};
        m_highlightPath = rails::RailPath(highlightStyle);

        if (std::abs(highlightLanePoint.y - railEntryPoint.y) > 0.001f) {
            m_highlightPath.appendStraight(railEntryPoint, highlightLanePoint);
        }
        if (std::abs(path.turnEntryPosition.x - highlightLanePoint.x) > 0.001f ||
            std::abs(path.turnEntryPosition.y - highlightLanePoint.y) > 0.001f) {
            m_highlightPath.appendStraight(highlightLanePoint, path.turnEntryPosition);
        }
        if (path.turnRadius > 0.001f && std::abs(path.turnEndAngle - path.turnStartAngle) > 0.001f) {
            m_highlightPath.appendArc(path.turnCenter, path.turnRadius, path.turnStartAngle, path.turnEndAngle);
        }
        if (std::abs(path.collectorPosition.x - path.turnExitPosition.x) > 0.001f ||
            std::abs(path.collectorPosition.y - path.turnExitPosition.y) > 0.001f) {
            m_highlightPath.appendStraight(path.turnExitPosition, path.collectorPosition);
        }
        if (path.junctionTurnRadius > 0.001f &&
            std::abs(path.junctionTurnEndAngle - path.junctionTurnStartAngle) > 0.001f) {
            m_highlightPath.appendArc(path.junctionTurnCenter,
                                      path.junctionTurnRadius,
                                      path.junctionTurnStartAngle,
                                      path.junctionTurnEndAngle);
        }
        if (std::abs(path.exitPosition.x - path.junctionTurnExitPosition.x) > 0.001f ||
            std::abs(path.exitPosition.y - path.junctionTurnExitPosition.y) > 0.001f) {
            m_highlightPath.appendStraight(path.junctionTurnExitPosition, path.exitPosition);
        }
    }
}

void RamView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_outputPort, states);

    for (const rails::RailPath& path : m_railPaths) {
        target.draw(path, states);
    }

    if (!m_highlightPath.isEmpty()) {
        target.draw(m_highlightPath, states);
    }

    for (const CacheLineView& line : m_lines) {
        target.draw(line, states);
    }
}
} // namespace view
