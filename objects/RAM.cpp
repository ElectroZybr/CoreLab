#include "RAM.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace {
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr sf::Vector2f kSlotSize{CacheLine::kWidth, CacheLine::kHeight};
constexpr float kColumnGap = 20.0f;
constexpr float kRowGap = 110.0f;
constexpr float kBusObjectInsetX = 36.0f;
constexpr float kBusLaneWidth = CacheLine::kWidth + 72.0f;
constexpr float kLeftPadding = kBusLaneWidth + 24.0f;
constexpr float kRightPadding = 24.0f;
constexpr float kTopPadding = 18.0f;
constexpr float kBottomPadding = 24.0f;
constexpr float kSlotsTopOffset = 74.0f;
constexpr float kMinimumWidth = 320.0f;
constexpr std::size_t kMaxColumns = 8;
constexpr float kTrackThickness = 6.0f;
constexpr float kTrackTurnRadius = 200.0f;
constexpr std::size_t kTrackTurnSegments = 10;
constexpr float kExitOffsetY = 0.0f;
const sf::Color kTrackColor(116, 134, 165);

std::size_t ceilDiv(std::size_t value, std::size_t divisor) {
    if (divisor == 0) {
        return 0;
    }

    return (value + divisor - 1) / divisor;
}

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
    const std::size_t rows = ceilDiv(slotCount, columns);

    const float slotsWidth =
        static_cast<float>(columns) * kSlotSize.x + static_cast<float>(columns - 1) * kColumnGap;

    const float slotsHeight = static_cast<float>(rows) * kSlotSize.y + static_cast<float>(rows) * kRowGap;

    return {std::max(kMinimumWidth, kLeftPadding + slotsWidth + kRightPadding),
            kSlotsTopOffset + slotsHeight + kBottomPadding};
}

float computeLaneTop(float rowY) {
    return rowY + kSlotSize.y + (kRowGap - kSlotSize.y) * 0.5f;
}

sf::VertexArray buildTrackBend(
    sf::Vector2f center, float radius, float thickness, float startAngle, float endAngle, sf::Color color) {
    sf::VertexArray bend(sf::PrimitiveType::TriangleStrip);

    const float innerRadius = radius - thickness * 0.5f;
    const float outerRadius = radius + thickness * 0.5f;

    for (std::size_t step = 0; step <= kTrackTurnSegments; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(kTrackTurnSegments);
        const float angle = startAngle + (endAngle - startAngle) * t;
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);

        bend.append(sf::Vertex({center.x + cosine * outerRadius, center.y + sine * outerRadius}, color));
        bend.append(sf::Vertex({center.x + cosine * innerRadius, center.y + sine * innerRadius}, color));
    }

    return bend;
}
} // namespace

RAM::RAM(std::size_t sizeInBytes, const sf::Font* font) : m_font(font), m_sizeInBytes(sizeInBytes) {
    m_slotCount = ceilDiv(m_sizeInBytes, kCacheLineSizeInBytes);
    rebuildGeometry();
    rebuildText();
    layout();
}

void RAM::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

sf::Vector2f RAM::getLinePosition(std::size_t index) const {
    if (index >= m_lines.size()) {
        return m_position;
    }

    return m_lines[index].getPosition();
}

RAM::ReadPath RAM::getReadPath(std::size_t index) const {
    if (index >= m_lines.size()) {
        return {m_position, m_position, m_position, m_position, 0.0f, m_position, m_position};
    }

    const sf::Vector2f sourcePosition = m_lines[index].getPosition();
    const sf::Vector2f lanePosition{sourcePosition.x, computeLaneTop(sourcePosition.y)};
    const float busObjectX = m_position.x + kBusObjectInsetX;
    const float laneCenterY = lanePosition.y + CacheLine::kHeight * 0.5f;
    const sf::Vector2f turnEntryPosition{busObjectX + kTrackTurnRadius, lanePosition.y};
    const sf::Vector2f turnCenter{busObjectX + CacheLine::kWidth * 0.5f + kTrackTurnRadius,
                                  laneCenterY - kTrackTurnRadius};
    const sf::Vector2f turnExitPosition{busObjectX, lanePosition.y - kTrackTurnRadius};
    const sf::Vector2f exitPosition{turnExitPosition.x, m_position.y - CacheLine::kHeight - kExitOffsetY};

    return {sourcePosition,
            lanePosition,
            turnEntryPosition,
            turnCenter,
            kTrackTurnRadius,
            turnExitPosition,
            exitPosition};
}

void RAM::setFont(const sf::Font* font) {
    m_font = font;

    for (CacheLine& line : m_lines) {
        line.setFont(font);
    }

    rebuildText();
    layout();
}

void RAM::rebuildGeometry() {
    m_size = computeRamSize(m_slotCount);

    buildRoundedRect(m_container, m_size, kCornerRadius);
    m_container.setFillColor(sf::Color(36, 44, 60));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(88, 112, 150));

    m_lines.clear();
    m_lines.reserve(m_slotCount);
    for (std::size_t index = 0; index < m_slotCount; ++index) {
        m_lines.emplace_back(m_font);
    }
}

void RAM::rebuildText() {
    m_titleText.reset();

    if (!m_font) {
        return;
    }

    m_titleText.emplace(*m_font, "RAM " + std::to_string(m_sizeInBytes) + " B", 30);
    m_titleText->setFillColor(sf::Color::White);
}

void RAM::layout() {
    m_container.setPosition(m_position);

    if (m_titleText) {
        m_titleText->setPosition(m_position + sf::Vector2f(24.0f, kTopPadding));
    }

    if (m_lines.empty()) {
        return;
    }

    m_tracks.clear();
    m_trackBends.clear();

    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    const std::size_t rows = ceilDiv(m_slotCount, columns);
    const float busObjectX = m_position.x + kBusObjectInsetX;
    const float busCenterX = busObjectX + CacheLine::kWidth * 0.5f;
    const float lastLaneCenterY = m_position.y + kSlotsTopOffset +
                                  static_cast<float>(rows - 1) * (kSlotSize.y + kRowGap) + kSlotSize.y +
                                  (kRowGap - kSlotSize.y) * 0.5f + kSlotSize.y * 0.5f;
    const float exitCenterY = m_position.y - kExitOffsetY - kSlotSize.y * 0.5f;

    sf::RectangleShape busTrack;
    busTrack.setPosition({busCenterX - kTrackThickness * 0.5f, exitCenterY});
    busTrack.setSize({kTrackThickness, lastLaneCenterY - exitCenterY});
    busTrack.setFillColor(kTrackColor);
    m_tracks.push_back(busTrack);

    for (std::size_t row = 0; row < rows; ++row) {
        const float rowY = m_position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);

        const float laneTopY = computeLaneTop(rowY);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;
        const float laneEndX = m_position.x + kLeftPadding +
                               static_cast<float>(columns - 1) * (kSlotSize.x + kColumnGap) +
                               kSlotSize.x * 0.5f;

        sf::RectangleShape horizontalTrack;
        horizontalTrack.setPosition({busCenterX + kTrackTurnRadius, laneCenterY - kTrackThickness * 0.5f});
        horizontalTrack.setSize({laneEndX - (busCenterX + kTrackTurnRadius), kTrackThickness});
        horizontalTrack.setFillColor(kTrackColor);
        m_tracks.push_back(horizontalTrack);

        m_trackBends.push_back(buildTrackBend({busCenterX + kTrackTurnRadius, laneCenterY - kTrackTurnRadius},
                                              kTrackTurnRadius,
                                              kTrackThickness,
                                              std::numbers::pi_v<float> * 0.5f,
                                              std::numbers::pi_v<float>,
                                              kTrackColor));
    }

    for (std::size_t index = 0; index < m_lines.size(); ++index) {
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;

        const float x = m_position.x + kLeftPadding + static_cast<float>(column) * (kSlotSize.x + kColumnGap);

        const float y = m_position.y + kSlotsTopOffset + static_cast<float>(row) * (kSlotSize.y + kRowGap);
        const float laneTopY = computeLaneTop(y);
        const float laneCenterY = laneTopY + kSlotSize.y * 0.5f;

        m_lines[index].setPosition({x, y});

        sf::RectangleShape verticalTrack;
        verticalTrack.setPosition({x + kSlotSize.x * 0.5f - kTrackThickness * 0.5f, y + kSlotSize.y});
        verticalTrack.setSize({kTrackThickness, laneCenterY - (y + kSlotSize.y)});
        verticalTrack.setFillColor(kTrackColor);
        m_tracks.push_back(verticalTrack);
    }
}

void RAM::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);

    for (const sf::RectangleShape& track : m_tracks) {
        target.draw(track, states);
    }

    for (const sf::VertexArray& bend : m_trackBends) {
        target.draw(bend, states);
    }

    for (const CacheLine& line : m_lines) {
        target.draw(line, states);
    }

    if (m_titleText) {
        target.draw(*m_titleText, states);
    }
}
