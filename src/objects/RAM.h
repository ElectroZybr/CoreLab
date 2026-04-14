#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <optional>

class RAM : public sf::Drawable
{
public:
    explicit RAM(std::size_t sizeInBytes, const sf::Font* font = nullptr);

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const { return m_position; }
    [[nodiscard]] std::size_t getSizeInBytes() const { return m_sizeInBytes; }

    void setFont(const sf::Font* font);

private:
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    std::size_t m_sizeInBytes = 0;
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::ConvexShape m_container;
    std::optional<sf::Text> m_titleText;
};
