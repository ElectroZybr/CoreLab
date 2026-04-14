#include "RAM.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace
{
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr sf::Vector2f kSlotSize{CacheLine::kWidth, CacheLine::kHeight};
constexpr float kColumnGap = 20.0f;
constexpr float kRowGap = 110.0f;
constexpr float kHorizontalPadding = 24.0f;
constexpr float kTopPadding = 18.0f;
constexpr float kBottomPadding = 24.0f;
constexpr float kSlotsTopOffset = 74.0f;
constexpr float kMinimumWidth = 320.0f;
constexpr std::size_t kMaxColumns = 8;

std::size_t ceilDiv(std::size_t value, std::size_t divisor)
{
    if (divisor == 0)
    {
        return 0;
    }

    return (value + divisor - 1) / divisor;
}

void buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius)
{
    constexpr float halfPi = std::numbers::pi_v<float> * 0.5f;
    constexpr std::array<float, 4> arcOffsets{
        std::numbers::pi_v<float>,
        std::numbers::pi_v<float> * 1.5f,
        0.0f,
        halfPi
    };

    const std::array<sf::Vector2f, 4> centers{
        sf::Vector2f(radius, radius),
        sf::Vector2f(size.x - radius, radius),
        sf::Vector2f(size.x - radius, size.y - radius),
        sf::Vector2f(radius, size.y - radius)
    };

    shape.setPointCount(arcOffsets.size() * kCornerPointCount);
    std::size_t pointIndex = 0;
    for (std::size_t cornerIndex = 0; cornerIndex < centers.size(); ++cornerIndex)
    {
        for (int step = 0; step < kCornerPointCount; ++step)
        {
            const float t = static_cast<float>(step) / static_cast<float>(kCornerPointCount - 1);
            const float angle = arcOffsets[cornerIndex] + t * halfPi;
            shape.setPoint(
                pointIndex++,
                {
                    centers[cornerIndex].x + std::cos(angle) * radius,
                    centers[cornerIndex].y + std::sin(angle) * radius
                }
            );
        }
    }
}

sf::Vector2f computeRamSize(std::size_t slotCount)
{
    if (slotCount == 0)
    {
        return {kMinimumWidth, kSlotsTopOffset + kBottomPadding};
    }

    const std::size_t columns = std::min(slotCount, kMaxColumns);
    const std::size_t rows = ceilDiv(slotCount, columns);

    const float slotsWidth =
        static_cast<float>(columns) * kSlotSize.x +
        static_cast<float>(columns - 1) * kColumnGap;

    const float slotsHeight =
        static_cast<float>(rows) * kSlotSize.y +
        static_cast<float>(rows - 1) * kRowGap;

    return {
        std::max(kMinimumWidth, slotsWidth + kHorizontalPadding * 2.0f),
        kSlotsTopOffset + slotsHeight + kBottomPadding
    };
}
}

RAM::RAM(std::size_t sizeInBytes, const sf::Font* font) : m_font(font), m_sizeInBytes(sizeInBytes)
{
    m_slotCount = ceilDiv(m_sizeInBytes, kCacheLineSizeInBytes);
    rebuildGeometry();
    rebuildText();
    layout();
}

void RAM::setPosition(sf::Vector2f position)
{
    m_position = position;
    layout();
}

void RAM::setFont(const sf::Font* font)
{
    m_font = font;

    for (CacheLine& line : m_lines)
    {
        line.setFont(font);
    }

    rebuildText();
    layout();
}

void RAM::rebuildGeometry()
{
    m_size = computeRamSize(m_slotCount);

    buildRoundedRect(m_container, m_size, kCornerRadius);
    m_container.setFillColor(sf::Color(36, 44, 60));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(88, 112, 150));

    m_lines.clear();
    m_lines.reserve(m_slotCount);
    for (std::size_t index = 0; index < m_slotCount; ++index)
    {
        m_lines.emplace_back(m_font);
    }
}

void RAM::rebuildText()
{
    m_titleText.reset();

    if (!m_font)
    {
        return;
    }

    m_titleText.emplace(*m_font, "RAM " + std::to_string(m_sizeInBytes) + " B", 30);
    m_titleText->setFillColor(sf::Color::White);
}

void RAM::layout()
{
    m_container.setPosition(m_position);

    if (m_titleText)
    {
        m_titleText->setPosition(m_position + sf::Vector2f(kHorizontalPadding, kTopPadding));
    }

    if (m_lines.empty())
    {
        return;
    }

    const std::size_t columns = std::min(m_slotCount, kMaxColumns);
    for (std::size_t index = 0; index < m_lines.size(); ++index)
    {
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;

        const float x =
            m_position.x + kHorizontalPadding +
            static_cast<float>(column) * (kSlotSize.x + kColumnGap);

        const float y =
            m_position.y + kSlotsTopOffset +
            static_cast<float>(row) * (kSlotSize.y + kRowGap);

        m_lines[index].setPosition({x, y});
    }
}

void RAM::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_container, states);

    for (const CacheLine& line : m_lines)
    {
        target.draw(line, states);
    }

    if (m_titleText)
    {
        target.draw(*m_titleText, states);
    }
}
