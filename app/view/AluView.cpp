#include "view/AluView.h"

namespace {
constexpr sf::Vector2f kFallbackSize{1200.0f, 1200.0f};
}

namespace view {
AluView::AluView(const sf::Font* font, sf::Vector2f position, sf::Vector2f size)
    : BlockView(font,
                position,
                size.x > 0.0f && size.y > 0.0f ? size : kFallbackSize) {
    setHeaderLayout({84.0f, 18.0f, 98.0f, 72, 18});
    setTitle("ALU");
    setSubtitle("vector + scalar execution");
    layoutBlock();
}

void AluView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {}
} // namespace view
