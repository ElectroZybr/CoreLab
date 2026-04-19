#pragma once

#include <SFML/Graphics.hpp>

class Scene : public sf::Drawable {
  public:
    Scene();
    [[nodiscard]] const sf::Font* getFont() const {
        return m_hasFont ? &m_font : nullptr;
    }

  private:
    void buildGrid();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::VertexArray m_grid;
    sf::Font m_font;
    bool m_hasFont = false;
};
