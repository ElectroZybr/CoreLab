#include "view/CpuView.h"

#include "view/CoreView.h"

#include <string>

namespace {
constexpr sf::Vector2f kSize{3000.0f, 1700.0f};
constexpr sf::Vector2f kInitialCoreSize{1420.0f, 1450.0f};
constexpr float kContentPadding = 34.0f;
constexpr float kCoreGap = 28.0f;
constexpr float kTopContentY = 150.0f;
constexpr float kIoPortWidth = 52.0f;
constexpr float kIoPortHeight = 28.0f;
const sf::Color kTransparentPortColor(0, 0, 0, 0);
} // namespace

namespace view {
CpuView::CpuView(const sf::Font* font, sf::Vector2f position) : BlockView(font, position, kSize) {
    setHeaderLayout({88.0f, 20.0f, 106.0f, 82, 20});
    setTitle("CPU");
    setSubtitle("2 cores  |  core 0 active, core 1 reserved");
    addPort("cache_in", PortKind::Input, PortDirection::Right, PayloadKind::CacheLine);
    rebuildCores();
    layoutBlock();
}

void CpuView::syncPrimaryCache(const sim::Cache& cache,
                               const sim::RAM* ram,
                               const sim::MemoryTransaction* activeTransaction) {
    if (CoreView* core = getPrimaryCore()) {
        core->syncCache(cache, ram, activeTransaction);
    }
}

CacheView* CpuView::getPrimaryCacheView() {
    if (CoreView* core = getPrimaryCore()) {
        return core->getCacheView();
    }

    return nullptr;
}

const CacheView* CpuView::getPrimaryCacheView() const {
    if (const CoreView* core = getPrimaryCore()) {
        return core->getCacheView();
    }

    return nullptr;
}

void CpuView::rebuildCores() {
    if (!m_cores.empty()) {
        return;
    }

    for (std::size_t index = 0; index < 2; ++index) {
        const bool active = index == 0;
        auto core = std::make_unique<CoreView>(
            "Core " + std::to_string(index), active, getFont(), sf::Vector2f{}, kInitialCoreSize);
        addChild(*core);
        m_cores.push_back(std::move(core));
    }
}

void CpuView::layoutBlock() {
    BlockView::layoutBlock();
    rebuildCores();

    const sf::Vector2f size = getBlockSize();

    if (PortView* cacheIn = findPort("cache_in")) {
        cacheIn->setLocalAnchor({size.x, size.y * 0.5f});
        cacheIn->setSize({kIoPortWidth, kIoPortHeight});
        cacheIn->setColors(kTransparentPortColor, kTransparentPortColor);
    }

    if (m_cores.empty()) {
        return;
    }

    const float coreWidth = (size.x - kContentPadding * 2.0f - kCoreGap) * 0.5f;
    const float coreHeight = size.y - kTopContentY - kContentPadding;

    for (std::size_t index = 0; index < m_cores.size(); ++index) {
        m_cores[index]->setCoreSize({coreWidth, coreHeight});
        const sf::Vector2f corePosition{
            kContentPadding + static_cast<float>(index) * (coreWidth + kCoreGap),
            kTopContentY,
        };
        m_cores[index]->setPosition(corePosition);
    }
}

void CpuView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {}
} // namespace view
