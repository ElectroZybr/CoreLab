#include "CacheLine.h"

#include <cmath>
#include <numbers>

namespace
{
constexpr sf::Vector2f kSlotSize(94.0f, 82.0f);
constexpr float kCornerRadius = 16.0f;
constexpr int kCornerPointCount = 6;
constexpr float kDividerWidth = 2.0f;

constexpr std::array<const char*, CacheLine::kFloatCount> kDefaultLabels{
    "x", "y", "z", "vx", "vy", "vz", "...", "..."
};

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

void centerText(sf::Text& text, const sf::FloatRect& rect)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({
        rect.position.x + (rect.size.x - bounds.size.x) * 0.5f - bounds.position.x,
        rect.position.y + (rect.size.y - bounds.size.y) * 0.5f - bounds.position.y
    });
}
}

CacheLine::CacheLine(const sf::Font* font) : m_font(font)
{
    for (std::size_t index = 0; index < m_labels.size(); ++index)
    {
        m_labels[index] = kDefaultLabels[index];
    }

    buildRoundedRect(m_container, {kFloatCount * kSlotSize.x, kSlotSize.y}, kCornerRadius);
    m_container.setFillColor(sf::Color(200, 210, 223));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(70, 97, 138));

    for (sf::RectangleShape& divider : m_dividers)
    {
        divider.setSize({kDividerWidth, kSlotSize.y});
        divider.setFillColor(sf::Color(70, 97, 138));
    }

    rebuildTexts();
    layout();
}

void CacheLine::setPosition(sf::Vector2f position)
{
    m_position = position;
    layout();
}

void CacheLine::setFont(const sf::Font* font)
{
    m_font = font;
    rebuildTexts();
    layout();
}

void CacheLine::setLabel(std::size_t index, const sf::String& label)
{
    if (index >= m_labels.size())
    {
        return;
    }

    m_labels[index] = label;
    rebuildTexts();
    layout();
}

void CacheLine::setLabels(const std::array<sf::String, kFloatCount>& labels)
{
    m_labels = labels;
    rebuildTexts();
    layout();
}

sf::FloatRect CacheLine::getBounds() const
{
    return {m_position, {kFloatCount * kSlotSize.x, kSlotSize.y}};
}

void CacheLine::rebuildTexts()
{
    for (std::optional<sf::Text>& slotText : m_slotTexts)
    {
        slotText.reset();
    }

    if (!m_font)
    {
        return;
    }

    for (std::size_t index = 0; index < m_slotTexts.size(); ++index)
    {
        m_slotTexts[index].emplace(*m_font, m_labels[index], 28);
        m_slotTexts[index]->setFillColor(sf::Color(27, 40, 67));
    }
}

void CacheLine::layout()
{
    m_container.setPosition(m_position);

    for (std::size_t index = 0; index < m_slotTexts.size(); ++index)
    {
        const float slotX = m_position.x + index * kSlotSize.x;

        if (m_slotTexts[index])
        {
            centerText(*m_slotTexts[index], {sf::Vector2f(slotX, m_position.y), kSlotSize});
        }
    }

    for (std::size_t index = 0; index < m_dividers.size(); ++index)
    {
        const float dividerX = m_position.x + (index + 1) * kSlotSize.x - kDividerWidth * 0.5f;
        m_dividers[index].setPosition({dividerX, m_position.y});
    }
}

void CacheLine::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_container, states);

    for (const sf::RectangleShape& divider : m_dividers)
    {
        target.draw(divider, states);
    }

    for (std::size_t index = 0; index < m_slotTexts.size(); ++index)
    {
        if (m_slotTexts[index])
        {
            target.draw(*m_slotTexts[index], states);
        }
    }
}
