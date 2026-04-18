#pragma once

#include <SFML/Graphics.hpp>

#include "view/rails/RailPath.h"
#include "view/rails/RailBuilder.h"

namespace view {
class BusView : public sf::Drawable {
  public:
    explicit BusView(float thickness = 10.0f, float turnRadius = 110.0f);

    void setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft);
    void setEndpoints(
        sf::Vector2f startTopLeft, sf::Vector2f endTopLeft, rails::RailDirection endDirection);
    void setCenterEndpoints(sf::Vector2f startCenter, sf::Vector2f endCenter);
    void setCenterEndpoints(
        sf::Vector2f startCenter, sf::Vector2f endCenter, rails::RailDirection endDirection);
    void setHighlighted(bool highlighted) {
        m_highlighted = highlighted;
    }
    void clear();
    [[nodiscard]] bool isVisible() const {
        return m_visible;
    }
    [[nodiscard]] const rails::RailPath& getPath() const {
        return m_path;
    }
    [[nodiscard]] sf::Vector2f sampleTopLeft(float progress) const;

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    float m_thickness = 10.0f;
    float m_turnRadius = 110.0f;
    bool m_visible = false;
    bool m_highlighted = false;
    rails::RailStyle m_style;
    rails::RailStyle m_highlightStyle;
    rails::RailPath m_path;
    rails::RailPath m_highlightPath;
};
} // namespace view
