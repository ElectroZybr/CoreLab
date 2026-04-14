#pragma once

#include <SFML/Graphics.hpp>

#include "objects/CacheLine.h"

class Scene : public sf::Drawable
{
public:
    Scene();
    [[nodiscard]] const sf::Font* getFont() const { return m_hasFont ? &m_font : nullptr; }
    [[nodiscard]] sf::Vector2f getCacheLinePosition() const { return m_cacheLine.getPosition(); }
    [[nodiscard]] sf::Vector2f getCacheLineEntryPosition() const
    {
        return {
            m_cacheLine.getPosition().x - CacheLine::kWidth,
            m_cacheLine.getPosition().y
        };
    }

private:
    void buildGrid();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::VertexArray m_grid;
    sf::Font m_font;
    bool m_hasFont = false;
    CacheLine m_cacheLine;
};
