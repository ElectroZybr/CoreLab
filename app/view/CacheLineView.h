#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace view {
class CacheLineView : public sf::Drawable {
  public:
    static constexpr float kWidth = 752.0f;
    static constexpr float kHeight = 82.0f;
    static constexpr std::size_t kFloatCount = 16;

    explicit CacheLineView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const {
        return m_position;
    }
    [[nodiscard]] sf::Vector2f getEntryPosition() const {
        return {m_position.x - kWidth, m_position.y};
    }

    void setFont(const sf::Font* font);
    void setCellLabels(const std::array<std::string, kFloatCount>& labels);

  private:
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_position{0.0f, 0.0f};
    std::array<std::string, kFloatCount> m_labels;
    sf::ConvexShape m_container;
    std::array<sf::RectangleShape, kFloatCount - 1> m_dividers;
    std::array<std::optional<sf::Text>, kFloatCount> m_cellTexts;
};
} // namespace view
