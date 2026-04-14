#include "view/CacheView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace
{
constexpr float kWidth = view::CacheLineView::kWidth + 48.0f;
constexpr float kHeight = view::CacheLineView::kHeight + 82.0f;
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr float kHorizontalPadding = 24.0f;
constexpr float kTitleOffsetX = 24.0f;
constexpr float kTitleOffsetY = 14.0f;
constexpr float kLineOffsetY = 54.0f;
constexpr unsigned int kTitleTextSize = 26;

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

namespace view
{
CacheView::CacheView(const sf::Font* font) : m_font(font), m_lineView(font)
{
    buildRoundedRect(m_container, {kWidth, kHeight}, kCornerRadius);
    m_container.setFillColor(sf::Color(29, 36, 49));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(82, 107, 144));

    rebuildText();
    layout();
}

void CacheView::setPosition(sf::Vector2f position)
{
    m_position = position;
    layout();
}

sf::Vector2f CacheView::getLinePosition() const
{
    return m_lineView.getPosition();
}

sf::Vector2f CacheView::getEntryPosition() const
{
    return m_lineView.getEntryPosition();
}

void CacheView::setFont(const sf::Font* font)
{
    m_font = font;
    m_lineView.setFont(font);
    rebuildText();
    layout();
}

void CacheView::rebuildText()
{
    m_titleText.reset();

    if (!m_font)
    {
        return;
    }

    m_titleText.emplace(*m_font, "L1 Cache", kTitleTextSize);
    m_titleText->setFillColor(sf::Color::White);
}

void CacheView::layout()
{
    m_container.setPosition(m_position);
    m_lineView.setPosition({
        m_position.x + kHorizontalPadding,
        m_position.y + kLineOffsetY
    });

    if (m_titleText)
    {
        m_titleText->setPosition({
            m_position.x + kTitleOffsetX,
            m_position.y + kTitleOffsetY
        });
    }
}

void CacheView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_container, states);

    if (m_titleText)
    {
        target.draw(*m_titleText, states);
    }

    target.draw(m_lineView, states);
}
}
