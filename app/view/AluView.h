#pragma once

#include <SFML/Graphics.hpp>

#include <optional>

#include "view/BlockView.h"
#include "view/CacheLineView.h"

namespace view {
class AluView : public BlockView {
  public:
    explicit AluView(const sf::Font* font = nullptr,
                     sf::Vector2f position = {0.0f, 0.0f},
                     sf::Vector2f size = {0.0f, 0.0f});

    void setViewSize(sf::Vector2f size) {
        setBlockSize(size);
    }
    [[nodiscard]] sf::Vector2f getViewSize() const {
        return getBlockSize();
    }

  private:
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;

    CacheLineView zmmRegister;
    std::optional<sf::Text> registerLabel;
};
} // namespace view
