#pragma once

#include <SFML/Graphics.hpp>

#include "view/BlockView.h"

namespace view {
class AluView : public BlockView {
  public:
    explicit AluView(const sf::Font* font = nullptr,
                     sf::Vector2f position = {0.0f, 0.0f},
                     sf::Vector2f size = {0.0f, 0.0f});

    void setViewSize(sf::Vector2f size) {
        setBlockSize(size);
    }

  private:
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;
};
} // namespace view
