#pragma once

#include <SFML/Graphics.hpp>

#include "view/rails/RailPath.h"

namespace view {
class BusView : public sf::Drawable {
  public:
    explicit BusView(float thickness = 10.0f, float turnRadius = 110.0f);

    void setEndpoints(sf::Vector2f startTopLeft, sf::Vector2f endTopLeft);
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
    rails::RailStyle m_style;
    rails::RailPath m_path;
};
} // namespace view
