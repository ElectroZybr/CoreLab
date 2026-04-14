#include "view/RamView.h"

namespace view
{
RamView::RamView(std::size_t sizeInBytes, const sf::Font* font) : m_ram(sizeInBytes, font)
{
}

void RamView::setPosition(sf::Vector2f position)
{
    m_ram.setPosition(position);
}

sf::Vector2f RamView::getPosition() const
{
    return m_ram.getPosition();
}

sf::Vector2f RamView::getLinePosition(std::size_t index) const
{
    return m_ram.getLinePosition(index);
}

RamView::ReadPath RamView::getReadPath(std::size_t index) const
{
    return m_ram.getReadPath(index);
}

std::size_t RamView::getSizeInBytes() const
{
    return m_ram.getSizeInBytes();
}

std::size_t RamView::getSlotCount() const
{
    return m_ram.getSlotCount();
}

void RamView::setFont(const sf::Font* font)
{
    m_ram.setFont(font);
}

void RamView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_ram, states);
}
}
