#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

class CacheLine : public sf::Drawable
{
public:
    static constexpr float kWidth = 752.0f;
    static constexpr float kHeight = 82.0f;
    static constexpr std::size_t kFloatCount = 8;

    explicit CacheLine(const sf::Font* font = nullptr);

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const { return m_position; }

    void setFont(const sf::Font* font);

private:
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::ConvexShape m_container;
    std::array<sf::RectangleShape, kFloatCount - 1> m_dividers;
    std::array<std::optional<sf::Text>, kFloatCount> m_cellTexts;
};
