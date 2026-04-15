#pragma once

#include <SFML/Graphics.hpp>

namespace view {
class BusView : public sf::Drawable {
  public:
    explicit BusView(float thickness = 10.0f);

    void setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft);
    void clear();
    [[nodiscard]] bool isVisible() const {
        return m_visible;
    }

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    float m_thickness = 10.0f;
    bool m_visible = false;
    sf::RectangleShape m_body;
    sf::CircleShape m_startCap;
    sf::CircleShape m_endCap;
};
} // namespace view
