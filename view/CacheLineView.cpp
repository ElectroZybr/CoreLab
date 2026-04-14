#include "view/CacheLineView.h"

namespace view
{
CacheLineView::CacheLineView(const sf::Font* font) : m_cacheLine(font)
{
}

void CacheLineView::setPosition(sf::Vector2f position)
{
    m_cacheLine.setPosition(position);
}

sf::Vector2f CacheLineView::getPosition() const
{
    return m_cacheLine.getPosition();
}

sf::Vector2f CacheLineView::getEntryPosition() const
{
    return {
        m_cacheLine.getPosition().x - kWidth,
        m_cacheLine.getPosition().y
    };
}

void CacheLineView::setFont(const sf::Font* font)
{
    m_cacheLine.setFont(font);
}

void CacheLineView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_cacheLine, states);
}
}
