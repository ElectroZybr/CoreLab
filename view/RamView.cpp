#include "view/RamView.h"

#include "sim/Math.h"
#include "view/rails/ArcRailSegment.h"
#include "view/rails/StraightRailSegment.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <string>

namespace {
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr sf::Vector2f kSlotSize{view::CacheLineView::kWidth, view::CacheLineView::kHeight};
constexpr float kColumnGap = 20.0f;
constexpr float kRowGap = 110.0f;
constexpr float kBusObjectInsetX = 36.0f;
constexpr float kBusLaneWidth = view::CacheLineView::kWidth + 72.0f;
constexpr float kLeftPadding = kBusLaneWidth + 24.0f;
constexpr float kRightPadding = 24.0f;
constexpr float kTopPadding = 18.0f;
constexpr float kBottomPadding = 24.0f;
constexpr float kSlotsTopOffset = 74.0f;
constexpr float kMinimumWidth = 320.0f;
constexpr std::size_t kMaxColumns = 8;
constexpr float kTrackThickness = 6.0f;
constexpr float kCollectorTurnRadius = 84.0f;
constexpr float kOutputTrackOutsetX = 80.0f;
const sf::Color kTrackColor(116, 134, 165);

struct TangentBridge {
    bool valid = false;
    std::size_t row = 0;
    sf::Vector2f rowPoint{0.0f, 0.0f};
    float rowAngle = 0.0f;
    sf::Vector2f outputPoint{0.0f, 0.0f};
    float outputAngle = 0.0f;
};

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
    : m_font(font), m_sizeInBytes(sizeInBytes), m_position(position) {
    m_slotCount = math::ceilDiv(m_sizeInBytes, kCacheLineSizeInBytes);
    rebuildGeometry();
    rebuildText();
    layout();
}

void RamView::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

sf::Vector2f RamView::getLinePosition(std::size_t index) const {
    if (index >= m_lines.size()) {
        return m_position;
    }

    return m_lines[index].getPosition();
}

RamView::ReadPath RamView::getReadPath(std::size_t index) const {
    if (index >= m_lines.size()) {
        return {m_position,
                m_position,
                m_position,
                m_position,
                0.0f,
                0.0f,
                0.0f,
                m_position,
                m_position,
                m_position,
                0.0f,
                0.0f,
                0.0f,
                m_position,
                m_position};
    }

    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    const std::size_t rows = math::ceilDiv(m_slotCount, columns);
    const std::size_t row = index / columns;
    const sf::Vector2f sourcePosition = m_lines[index].getPosition();
    const sf::Vector2f lanePosition{sourcePosition.x, computeLaneTop(sourcePosition.y)};
    const float laneCenterY = lanePosition.y + CacheLineView::kHeight * 0.5f;
    const float busObjectX = m_position.x + kBusObjectInsetX;
    const float busCenterX = busObjectX + CacheLineView::kWidth * 0.5f;
    const float outputCenterX = m_position.x - kOutputTrackOutsetX;
    const float firstLaneCenterY =
        m_position.y + kSlotsTopOffset + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float lastLaneCenterY = m_position.y + kSlotsTopOffset +
                                  static_cast<float>(rows - 1) * (kSlotSize.y + kRowGap) + kSlotSize.y +
                                  (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float junctionCenterY = (firstLaneCenterY + lastLaneCenterY) * 0.5f;
    const std::optional<TangentBridge> upperTangent =
        findNearestUpperTangent(m_position, rows, busCenterX, junctionCenterY);
    const std::optional<TangentBridge> lowerTangent =
        findNearestLowerTangent(m_position, rows, busCenterX, junctionCenterY);
    sf::Vector2f turnEntryPosition{busObjectX + kCollectorTurnRadius, lanePosition.y};
    sf::Vector2f turnCenter{busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius};
    float turnStartAngle = std::numbers::pi_v<float> * 0.5f;
    float turnEndAngle = std::numbers::pi_v<float>;
    sf::Vector2f turnExitPosition{busObjectX, lanePosition.y - kCollectorTurnRadius};
    sf::Vector2f collectorPosition{busObjectX,
                                   junctionCenterY + kCollectorTurnRadius - CacheLineView::kHeight * 0.5f};
    sf::Vector2f junctionTurnCenter{busCenterX - kCollectorTurnRadius,
                                    junctionCenterY + kCollectorTurnRadius};
    float junctionTurnStartAngle = 0.0f;
    float junctionTurnEndAngle = -std::numbers::pi_v<float> * 0.5f;
    sf::Vector2f junctionTurnExitPosition{busObjectX - kCollectorTurnRadius,
                                          junctionCenterY - CacheLineView::kHeight * 0.5f};

    if (laneCenterY < junctionCenterY - 0.5f) {
        const bool isNearestUpper = upperTangent && upperTangent->row == row;

        turnCenter = {busCenterX + kCollectorTurnRadius, laneCenterY + kCollectorTurnRadius};
        turnStartAngle = -std::numbers::pi_v<float> * 0.5f;
        turnEndAngle = isNearestUpper ? upperTangent->rowAngle : -std::numbers::pi_v<float>;
        turnExitPosition = isNearestUpper
                               ? sf::Vector2f{upperTangent->rowPoint.x - CacheLineView::kWidth * 0.5f,
                                              upperTangent->rowPoint.y - CacheLineView::kHeight * 0.5f}
                               : sf::Vector2f{busObjectX, lanePosition.y + kCollectorTurnRadius};
        collectorPosition =
            isNearestUpper
                ? sf::Vector2f{upperTangent->outputPoint.x - CacheLineView::kWidth * 0.5f,
                               upperTangent->outputPoint.y - CacheLineView::kHeight * 0.5f}
                : sf::Vector2f{busObjectX,
                               junctionCenterY - kCollectorTurnRadius - CacheLineView::kHeight * 0.5f};
        junctionTurnCenter = {busCenterX - kCollectorTurnRadius, junctionCenterY - kCollectorTurnRadius};
        junctionTurnStartAngle = isNearestUpper ? upperTangent->outputAngle : 0.0f;
        junctionTurnEndAngle = std::numbers::pi_v<float> * 0.5f;
        junctionTurnExitPosition = {busObjectX - kCollectorTurnRadius,
                                    junctionCenterY - CacheLineView::kHeight * 0.5f};
    } else if (laneCenterY > junctionCenterY + 0.5f) {
        const bool isNearestLower = lowerTangent && lowerTangent->row == row;

        turnCenter = {busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius};
        turnStartAngle = std::numbers::pi_v<float> * 0.5f;
        turnEndAngle = isNearestLower ? lowerTangent->rowAngle : std::numbers::pi_v<float>;
        turnExitPosition = isNearestLower
                               ? sf::Vector2f{lowerTangent->rowPoint.x - CacheLineView::kWidth * 0.5f,
                                              lowerTangent->rowPoint.y - CacheLineView::kHeight * 0.5f}
                               : sf::Vector2f{busObjectX, lanePosition.y - kCollectorTurnRadius};
        collectorPosition =
            isNearestLower
                ? sf::Vector2f{lowerTangent->outputPoint.x - CacheLineView::kWidth * 0.5f,
                               lowerTangent->outputPoint.y - CacheLineView::kHeight * 0.5f}
                : sf::Vector2f{busObjectX,
                               junctionCenterY + kCollectorTurnRadius - CacheLineView::kHeight * 0.5f};
        junctionTurnCenter = {busCenterX - kCollectorTurnRadius, junctionCenterY + kCollectorTurnRadius};
        junctionTurnStartAngle = isNearestLower ? lowerTangent->outputAngle : 0.0f;
        junctionTurnEndAngle = -std::numbers::pi_v<float> * 0.5f;
        junctionTurnExitPosition = {busObjectX - kCollectorTurnRadius,
                                    junctionCenterY - CacheLineView::kHeight * 0.5f};
    } else {
        turnEntryPosition = {busObjectX, lanePosition.y};
        turnCenter = {busCenterX, laneCenterY};
        turnStartAngle = 0.0f;
        turnEndAngle = 0.0f;
        turnExitPosition = turnEntryPosition;
        collectorPosition = turnEntryPosition;
        junctionTurnCenter = {busCenterX, junctionCenterY};
        junctionTurnStartAngle = 0.0f;
        junctionTurnEndAngle = 0.0f;
        junctionTurnExitPosition = {busObjectX - kCollectorTurnRadius,
                                    junctionCenterY - CacheLineView::kHeight * 0.5f};
    }

    const sf::Vector2f exitPosition{outputCenterX - CacheLineView::kWidth * 0.5f,
                                    junctionCenterY - CacheLineView::kHeight * 0.5f};

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

void RamView::setFont(const sf::Font* font) {
    m_font = font;

    for (CacheLineView& line : m_lines) {
        line.setFont(font);
    }

    rebuildText();
    layout();
}

void RamView::rebuildGeometry() {
    m_size = computeRamSize(m_slotCount);

    m_container.setFillColor(sf::Color(36, 44, 60));
    m_container.setOutlineThickness(0.0f);
    buildRoundedRect(m_container, m_size, kCornerRadius);
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(88, 112, 150));

    m_lines.clear();
    m_lines.reserve(m_slotCount);
    for (std::size_t index = 0; index < m_slotCount; ++index) {
        m_lines.emplace_back(m_font);
    }
}

void RamView::rebuildText() {
    m_titleText.reset();

    if (!m_font) {
        return;
    }

    m_titleText.emplace(*m_font, "RAM " + std::to_string(m_sizeInBytes) + " B", 30);
    m_titleText->setFillColor(sf::Color::White);
}

void RamView::layout() {
    m_container.setPosition(m_position);

    if (m_titleText) {
        m_titleText->setPosition(m_position + sf::Vector2f(24.0f, kTopPadding));
    }

    if (m_lines.empty()) {
        return;
    }

    m_railSegments.clear();

    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    const std::size_t rows = math::ceilDiv(m_slotCount, columns);
    const float busObjectX = m_position.x + kBusObjectInsetX;
    const float busCenterX = busObjectX + CacheLineView::kWidth * 0.5f;
    const float outputCenterX = m_position.x - kOutputTrackOutsetX;
    const float firstLaneCenterY =
        m_position.y + kSlotsTopOffset + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float lastLaneCenterY = m_position.y + kSlotsTopOffset +
                                  static_cast<float>(rows - 1) * (kSlotSize.y + kRowGap) + kSlotSize.y +
                                  (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float junctionCenterY = (firstLaneCenterY + lastLaneCenterY) * 0.5f;
    const std::optional<TangentBridge> upperTangent =
        findNearestUpperTangent(m_position, rows, busCenterX, junctionCenterY);
    const std::optional<TangentBridge> lowerTangent =
        findNearestLowerTangent(m_position, rows, busCenterX, junctionCenterY);
    float upperCollectorMinY = std::numeric_limits<float>::max();
    float lowerCollectorMaxY = -std::numeric_limits<float>::max();
    bool hasUpperRows = false;
    bool hasLowerRows = false;
    bool hasFarUpperRows = false;
    bool hasFarLowerRows = false;

    for (std::size_t row = 0; row < rows; ++row) {
        const float rowY = m_position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
        const float laneTopY = computeLaneTop(rowY);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;
        const float laneEndX = m_position.x + kLeftPadding +
                               static_cast<float>(columns - 1) * (kSlotSize.x + kColumnGap) +
                               kSlotSize.x * 0.5f;

        if (laneCenterY < junctionCenterY - 0.5f) {
            const float turnExitCenterY = laneCenterY + kCollectorTurnRadius;
            const bool isNearestUpper = upperTangent && upperTangent->row == row;

            hasUpperRows = true;

            m_railSegments.push_back(std::make_unique<rails::StraightRailSegment>(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY},
                sf::Vector2f{laneEndX, laneCenterY},
                kTrackThickness,
                kTrackColor));

            m_railSegments.push_back(std::make_unique<rails::ArcRailSegment>(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY + kCollectorTurnRadius},
                kCollectorTurnRadius,
                -std::numbers::pi_v<float> * 0.5f,
                isNearestUpper ? upperTangent->rowAngle : -std::numbers::pi_v<float>,
                kTrackThickness,
                kTrackColor));

            if (isNearestUpper) {
                m_railSegments.push_back(std::make_unique<rails::StraightRailSegment>(
                    upperTangent->rowPoint, upperTangent->outputPoint, kTrackThickness, kTrackColor));
            } else {
                hasFarUpperRows = true;
                upperCollectorMinY = std::min(upperCollectorMinY, turnExitCenterY);
            }
        } else if (laneCenterY > junctionCenterY + 0.5f) {
            const float turnExitCenterY = laneCenterY - kCollectorTurnRadius;
            const bool isNearestLower = lowerTangent && lowerTangent->row == row;

            hasLowerRows = true;

            m_railSegments.push_back(std::make_unique<rails::StraightRailSegment>(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY},
                sf::Vector2f{laneEndX, laneCenterY},
                kTrackThickness,
                kTrackColor));

            m_railSegments.push_back(std::make_unique<rails::ArcRailSegment>(
                sf::Vector2f{busCenterX + kCollectorTurnRadius, laneCenterY - kCollectorTurnRadius},
                kCollectorTurnRadius,
                std::numbers::pi_v<float> * 0.5f,
                isNearestLower ? lowerTangent->rowAngle : std::numbers::pi_v<float>,
                kTrackThickness,
                kTrackColor));

            if (isNearestLower) {
                m_railSegments.push_back(std::make_unique<rails::StraightRailSegment>(
                    lowerTangent->rowPoint, lowerTangent->outputPoint, kTrackThickness, kTrackColor));
            } else {
                hasFarLowerRows = true;
                lowerCollectorMaxY = std::max(lowerCollectorMaxY, turnExitCenterY);
            }
        } else {
            m_railSegments.push_back(
                std::make_unique<rails::StraightRailSegment>(sf::Vector2f{busCenterX, laneCenterY},
                                                             sf::Vector2f{laneEndX, laneCenterY},
                                                             kTrackThickness,
                                                             kTrackColor));
        }
    }

    if (hasUpperRows) {
        const float upperCollectorEndY = junctionCenterY - kCollectorTurnRadius;

        if (hasFarUpperRows && upperCollectorMinY < upperCollectorEndY - 0.5f) {
            m_railSegments.push_back(
                std::make_unique<rails::StraightRailSegment>(sf::Vector2f{busCenterX, upperCollectorMinY},
                                                             sf::Vector2f{busCenterX, upperCollectorEndY},
                                                             kTrackThickness,
                                                             kTrackColor));
        }

        m_railSegments.push_back(std::make_unique<rails::ArcRailSegment>(
            sf::Vector2f{busCenterX - kCollectorTurnRadius, junctionCenterY - kCollectorTurnRadius},
            kCollectorTurnRadius,
            0.0f,
            std::numbers::pi_v<float> * 0.5f,
            kTrackThickness,
            kTrackColor));
    }

    if (hasLowerRows) {
        const float lowerCollectorStartY = junctionCenterY + kCollectorTurnRadius;

        if (hasFarLowerRows && lowerCollectorStartY < lowerCollectorMaxY - 0.5f) {
            m_railSegments.push_back(
                std::make_unique<rails::StraightRailSegment>(sf::Vector2f{busCenterX, lowerCollectorStartY},
                                                             sf::Vector2f{busCenterX, lowerCollectorMaxY},
                                                             kTrackThickness,
                                                             kTrackColor));
        }

        m_railSegments.push_back(std::make_unique<rails::ArcRailSegment>(
            sf::Vector2f{busCenterX - kCollectorTurnRadius, junctionCenterY + kCollectorTurnRadius},
            kCollectorTurnRadius,
            0.0f,
            -std::numbers::pi_v<float> * 0.5f,
            kTrackThickness,
            kTrackColor));
    }

    const float outputStartX = busCenterX - kCollectorTurnRadius;
    m_railSegments.push_back(
        std::make_unique<rails::StraightRailSegment>(sf::Vector2f{outputCenterX, junctionCenterY},
                                                     sf::Vector2f{outputStartX, junctionCenterY},
                                                     kTrackThickness,
                                                     kTrackColor));

    for (std::size_t index = 0; index < m_lines.size(); ++index) {
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;

        const float x = m_position.x + kLeftPadding + static_cast<float>(column) * (kSlotSize.x + kColumnGap);
        const float y = m_position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
        const float laneTopY = computeLaneTop(y);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;

        m_lines[index].setPosition({x, y});

        m_railSegments.push_back(std::make_unique<rails::StraightRailSegment>(
            sf::Vector2f{x + kSlotSize.x * 0.5f, y + kSlotSize.y},
            sf::Vector2f{x + kSlotSize.x * 0.5f, laneCenterY},
            kTrackThickness,
            kTrackColor));
    }
}

void RamView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);

    for (const std::unique_ptr<rails::RailSegment>& segment : m_railSegments) {
        target.draw(*segment, states);
    }

    for (const CacheLineView& line : m_lines) {
        target.draw(line, states);
    }

    if (m_titleText) {
        target.draw(*m_titleText, states);
    }
}
} // namespace view
