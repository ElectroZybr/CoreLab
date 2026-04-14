#include "CacheLine.h"

#include <array>
#include <cmath>
#include <numbers>

namespace
{
constexpr sf::Vector2f kSize{752.0f, 82.0f};
constexpr float kCornerRadius = 16.0f;
constexpr int kCornerPointCount = 8;

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
}

CacheLine::CacheLine(const sf::Font* font) : m_font(font)
{
    buildRoundedRect(m_container, kSize, kCornerRadius);
    m_container.setFillColor(sf::Color(200, 210, 223));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(70, 97, 138));

    rebuildText();
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
    rebuildText();
    layout();
}

void CacheLine::rebuildText()
{
    m_titleText.reset();

    if (!m_font)
    {
        return;
    }

    m_titleText.emplace(*m_font, "Cache line", 32);
    m_titleText->setFillColor(sf::Color(27, 40, 67));
}

void CacheLine::layout()
{
    m_container.setPosition(m_position);

    if (!m_titleText)
    {
        return;
    }

    const sf::FloatRect bounds = m_titleText->getLocalBounds();
    m_titleText->setPosition({
        m_position.x + (kSize.x - bounds.size.x) * 0.5f - bounds.position.x,
        m_position.y + (kSize.y - bounds.size.y) * 0.5f - bounds.position.y
    });
}

void CacheLine::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_container, states);

    if (m_titleText)
    {
        target.draw(*m_titleText, states);
    }
}
