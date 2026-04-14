#pragma once

#include <SFML/Graphics.hpp>

#include "view/CacheView.h"

class Scene : public sf::Drawable
{
public:
    Scene();
    [[nodiscard]] const sf::Font* getFont() const { return m_hasFont ? &m_font : nullptr; }
    [[nodiscard]] sf::Vector2f getCacheLinePosition() const { return m_cacheView.getLinePosition(); }
    [[nodiscard]] sf::Vector2f getCacheLineEntryPosition() const { return m_cacheView.getEntryPosition(); }

private:
    void buildGrid();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::VertexArray m_grid;
    sf::Font m_font;
    bool m_hasFont = false;
    view::CacheView m_cacheView;
};
