#include "view/CoreView.h"

#include <array>

namespace {
constexpr sf::Vector2f kFallbackSize{700.0f, 640.0f};
constexpr float kCornerRadius = 14.0f;
constexpr float kModuleGap = 16.0f;
constexpr float kContentTop = 98.0f;
constexpr float kContentPadding = 22.0f;
constexpr unsigned int kModuleTextSize = 18;

constexpr std::array<const char*, 4> kModuleLabels{
    "Control",
    "SIMD",
    "ALU",
    "Registers",
};

constexpr std::array<sf::Color, 4> kModuleFillColors{
    sf::Color(49, 62, 85),
    sf::Color(58, 77, 104),
    sf::Color(61, 82, 112),
    sf::Color(47, 56, 76),
};

const sf::Color kModuleOutlineColor(116, 134, 165);
const sf::Color kModuleTextColor(225, 238, 250);
} // namespace

namespace view {
CoreView::CoreView(std::string title, bool active, const sf::Font* font, sf::Vector2f position, sf::Vector2f size)
    : BlockView(font, position, size.x > 0.0f && size.y > 0.0f ? size : kFallbackSize), m_active(active) {
    setHeaderLayout({54.0f, 14.0f, 54.0f, 30, 14});
    setTitle(std::move(title));
    setSubtitle(m_active ? "private L1 + execution units" : "idle / reserved");

    if (m_active) {
        m_cacheView = std::make_unique<CacheView>(font);
        addChild(*m_cacheView);
    }

    rebuildModules();
    layoutBlock();
}

void CoreView::setCoreSize(sf::Vector2f size) {
    setBlockSize(size);
}

void CoreView::syncCache(const sim::Cache& cache,
                         const sim::RAM* ram,
                         const sim::MemoryTransaction* activeTransaction) {
    if (m_cacheView) {
        m_cacheView->sync(cache, ram, activeTransaction);
        layoutBlock();
    }
}

void CoreView::rebuildModules() {
    for (std::size_t index = 0; index < m_modules.size(); ++index) {
        m_modules[index].setOutlineThickness(0.0f);
        buildRoundedRect(m_modules[index], {0.0f, 0.0f}, kCornerRadius);
        m_modules[index].setOutlineThickness(3.0f);
        m_modules[index].setFillColor(kModuleFillColors[index]);
        m_modules[index].setOutlineColor(kModuleOutlineColor);
        m_moduleTexts[index].reset();
    }

    if (!getFont() || !m_active) {
        return;
    }

    for (std::size_t index = 0; index < m_moduleTexts.size(); ++index) {
        m_moduleTexts[index].emplace(*getFont(), kModuleLabels[index], kModuleTextSize);
        m_moduleTexts[index]->setFillColor(kModuleTextColor);
    }
}

void CoreView::layoutBlock() {
    BlockView::layoutBlock();
    rebuildModules();

    const sf::Vector2f worldPosition = getWorldPosition();
    const sf::Vector2f size = getBlockSize();

    if (!m_active) {
        return;
    }

    if (size.x <= kContentPadding * 2.0f + kModuleGap || size.y <= kContentTop + kContentPadding + kModuleGap) {
        return;
    }

    const float contentWidth = size.x - kContentPadding * 2.0f;
    const float contentHeight = size.y - kContentTop - kContentPadding;
    const float bottomRowHeight = (contentHeight - kModuleGap * 2.0f) * 0.34f;
    const float cacheHeight = contentHeight - bottomRowHeight - kModuleGap;
    const float halfWidth = (contentWidth - kModuleGap) * 0.5f;
    const float upperHeight = (bottomRowHeight - kModuleGap) * 0.52f;
    const float lowerHeight = bottomRowHeight - kModuleGap - upperHeight;

    if (m_cacheView) {
        m_cacheView->setPosition({kContentPadding, kContentTop});
    }

    const sf::Vector2f controlPosition{worldPosition.x + kContentPadding,
                                       worldPosition.y + kContentTop + cacheHeight + kModuleGap};
    const sf::Vector2f simdPosition{worldPosition.x + kContentPadding + halfWidth + kModuleGap,
                                    worldPosition.y + kContentTop + cacheHeight + kModuleGap};
    const sf::Vector2f aluPosition{worldPosition.x + kContentPadding, controlPosition.y + upperHeight + kModuleGap};
    const sf::Vector2f regsPosition{
        worldPosition.x + kContentPadding + halfWidth + kModuleGap,
        simdPosition.y + upperHeight + kModuleGap,
    };

    const std::array<sf::Vector2f, 4> modulePositions{
        controlPosition,
        simdPosition,
        aluPosition,
        regsPosition,
    };

    const std::array<sf::Vector2f, 4> moduleSizes{
        sf::Vector2f{halfWidth, upperHeight},
        sf::Vector2f{halfWidth, upperHeight},
        sf::Vector2f{halfWidth, lowerHeight},
        sf::Vector2f{halfWidth, lowerHeight},
    };

    for (std::size_t index = 0; index < m_modules.size(); ++index) {
        m_modules[index].setOutlineThickness(0.0f);
        buildRoundedRect(m_modules[index], moduleSizes[index], kCornerRadius);
        m_modules[index].setOutlineThickness(3.0f);
        m_modules[index].setPosition(modulePositions[index]);

        if (m_moduleTexts[index]) {
            sf::Text& text = *m_moduleTexts[index];
            const sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(
                {modulePositions[index].x + (moduleSizes[index].x - bounds.size.x) * 0.5f - bounds.position.x,
                 modulePositions[index].y + (moduleSizes[index].y - bounds.size.y) * 0.5f - bounds.position.y});
        }
    }
}

void CoreView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_active) {
        return;
    }

    for (std::size_t index = 0; index < m_modules.size(); ++index) {
        target.draw(m_modules[index], states);
        if (m_moduleTexts[index]) {
            target.draw(*m_moduleTexts[index], states);
        }
    }
}
} // namespace view
