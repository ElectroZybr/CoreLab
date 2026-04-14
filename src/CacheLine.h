#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

class CacheLine : public sf::Drawable
{
public:
    static constexpr std::size_t kByteSize = 64;
    static constexpr std::size_t kFloatCount = 8;

    explicit CacheLine(const sf::Font* font = nullptr);

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const { return m_position; }

    void setFont(const sf::Font* font);

    void setLabel(std::size_t index, const sf::String& label);
    void setLabels(const std::array<sf::String, kFloatCount>& labels);
    [[nodiscard]] const std::array<sf::String, kFloatCount>& getLabels() const { return m_labels; }

    [[nodiscard]] sf::FloatRect getBounds() const;

private:
    void rebuildTexts();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_position{0.0f, 0.0f};
    std::array<sf::String, kFloatCount> m_labels{};

    sf::ConvexShape m_container;
    std::array<sf::RectangleShape, kFloatCount - 1> m_dividers;
    std::array<std::optional<sf::Text>, kFloatCount> m_slotTexts;
};
