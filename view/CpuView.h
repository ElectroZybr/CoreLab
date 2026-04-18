#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

namespace view {
class CpuView : public sf::Drawable {
  public:
    explicit CpuView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const {
        return m_position;
    }
    [[nodiscard]] sf::FloatRect getBounds() const;
    [[nodiscard]] bool isInDragHandle(sf::Vector2f worldPoint) const;
    void setDragState(bool hovered, bool dragging);

    void setFont(const sf::Font* font);

  private:
    void rebuildGeometry();
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::Vector2f m_size{0.0f, 0.0f};
    sf::ConvexShape m_container;
    sf::ConvexShape m_dragHandleOverlay;
    std::vector<sf::RectangleShape> m_dragHandleMarks;
    bool m_dragHovered = false;
    bool m_dragging = false;
    std::array<sf::ConvexShape, 4> m_sections;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_summaryText;
    std::array<std::optional<sf::Text>, 4> m_sectionTexts;
};
} // namespace view
