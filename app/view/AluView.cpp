#include "view/AluView.h"

#include <array>

namespace {
constexpr sf::Vector2f kFallbackSize{1200.0f, 1200.0f};
constexpr float kRegisterTopOffset = 430.0f;
constexpr unsigned int kRegisterLabelSize = 28;
const sf::Color kRegisterLabelColor(225, 238, 250);
constexpr float kInputPortWidth = 52.0f;
constexpr float kInputPortHeight = 28.0f;
const sf::Color kTransparentPortColor(0, 0, 0, 0);
}

namespace view {
AluView::AluView(const sf::Font* font, sf::Vector2f position, sf::Vector2f size)
    : BlockView(font,
                position,
                size.x > 0.0f && size.y > 0.0f ? size : kFallbackSize),
      zmmRegister(font) {
    setHeaderLayout({84.0f, 18.0f, 98.0f, 72, 18});
    setTitle("ALU");
    setSubtitle("vector register + execution");
    addPort("zmm_in", PortKind::Input, PortDirection::Down, PayloadKind::CacheLine);

    std::array<std::string, CacheLineView::kFloatCount> emptyLabels{};
    emptyLabels.fill("");
    zmmRegister.setCellLabels(emptyLabels);

    if (font) {
        registerLabel.emplace(*font, "zmm0", kRegisterLabelSize);
        registerLabel->setFillColor(kRegisterLabelColor);
    }

    layoutBlock();
}

void AluView::layoutBlock() {
    BlockView::layoutBlock();

    const sf::Vector2f size = getBlockSize();
    const sf::Vector2f worldPosition = getWorldPosition();
    const sf::Vector2f registerPosition{
        worldPosition.x + size.x - CacheLineView::kWidth - 148.0f,
        worldPosition.y + kRegisterTopOffset,
    };
    zmmRegister.setPosition(registerPosition);

    if (registerLabel) {
        const sf::FloatRect bounds = registerLabel->getLocalBounds();
        registerLabel->setPosition(
            {registerPosition.x - bounds.position.x,
             registerPosition.y - 44.0f - bounds.position.y});
    }

    if (PortView* zmmIn = findPort("zmm_in")) {
        zmmIn->setLocalAnchor(
            {registerPosition.x - worldPosition.x + CacheLineView::kWidth,
             registerPosition.y - worldPosition.y + CacheLineView::kHeight * 0.5f});
        zmmIn->setSize({kInputPortWidth, kInputPortHeight});
        zmmIn->setColors(kTransparentPortColor, kTransparentPortColor);
        zmmIn->setDirection(PortDirection::Right);
    }
}

void AluView::setRegisterLabels(const std::array<std::string, CacheLineView::kFloatCount>& labels) {
    zmmRegister.setCellLabels(labels);
}

sf::Vector2f AluView::getRegisterCellCenter(std::size_t index) const {
    const std::size_t clampedIndex = std::min(index, CacheLineView::kFloatCount - 1);
    const float cellWidth = CacheLineView::kWidth / static_cast<float>(CacheLineView::kFloatCount);
    const sf::Vector2f position = zmmRegister.getPosition();
    return {position.x + cellWidth * (static_cast<float>(clampedIndex) + 0.5f),
            position.y + CacheLineView::kHeight * 0.5f};
}

void AluView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {
    if (registerLabel) {
        target.draw(*registerLabel, states);
    }
    target.draw(zmmRegister, states);
}
} // namespace view
