#pragma once

#include <SFML/Graphics.hpp>

#include "objects/CacheLine.h"

namespace view {
class CacheLineView : public sf::Drawable {
  public:
    static constexpr float kWidth = CacheLine::kWidth;
    static constexpr float kHeight = CacheLine::kHeight;

    explicit CacheLineView(const sf::Font* font = nullptr);

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const;
    [[nodiscard]] sf::Vector2f getEntryPosition() const;

    void setFont(const sf::Font* font);

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    CacheLine m_cacheLine;
};
} // namespace view
