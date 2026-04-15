#include "view/CpuView.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr sf::Vector2f kSize{230.0f, 96.0f};
constexpr float kCornerRadius = 18.0f;
constexpr int kCornerPointCount = 16;
constexpr unsigned int kTitleTextSize = 30;

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
} // namespace

namespace view {
CpuView::CpuView(const sf::Font* font, sf::Vector2f position) : m_font(font), m_position(position) {
    buildRoundedRect(m_container, kSize, kCornerRadius);
    m_container.setFillColor(sf::Color(44, 54, 74));
    m_container.setOutlineThickness(3.0f);
    m_container.setOutlineColor(sf::Color(92, 120, 164));

    rebuildText();
    layout();
}

void CpuView::setPosition(sf::Vector2f position) {
    m_position = position;
    layout();
}

void CpuView::setFont(const sf::Font* font) {
    m_font = font;
    rebuildText();
    layout();
}

void CpuView::rebuildText() {
    m_titleText.reset();

    if (!m_font) {
        return;
    }

    m_titleText.emplace(*m_font, "CPU", kTitleTextSize);
    m_titleText->setFillColor(sf::Color::White);
}

void CpuView::layout() {
    m_container.setPosition(m_position);

    if (!m_titleText) {
        return;
    }

    const sf::FloatRect bounds = m_titleText->getLocalBounds();
    m_titleText->setPosition({m_position.x + (kSize.x - bounds.size.x) * 0.5f - bounds.position.x,
                              m_position.y + (kSize.y - bounds.size.y) * 0.5f - bounds.position.y});
}

void CpuView::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_container, states);

    if (m_titleText) {
        target.draw(*m_titleText, states);
    }
}
} // namespace view
