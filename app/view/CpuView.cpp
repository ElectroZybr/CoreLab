#include "view/CpuView.h"

namespace {
constexpr sf::Vector2f kSize{3400.0f, 1800.0f};
constexpr float kContentPadding = 42.0f;
constexpr float kTopContentY = 170.0f;
constexpr float kContentGap = 26.0f;
constexpr float kIoPortWidth = 52.0f;
constexpr float kIoPortHeight = 28.0f;
const sf::Color kTransparentPortColor(0, 0, 0, 0);
} // namespace

namespace view {
CpuView::CpuView(const sf::Font* font, sf::Vector2f position)
    : BlockView(font, position, kSize) {
    setHeaderLayout({88.0f, 20.0f, 106.0f, 82, 20});
    setTitle("CPU");
    setSubtitle("L1 cache + execution pipeline");
    addPort("cache_in", PortKind::Input, PortDirection::Right, PayloadKind::CacheLine);
    rebuildUnits();
    layoutBlock();
}

void CpuView::syncPrimaryCache(const sim::Cache& cache,
                               const sim::RAM* ram,
                               const sim::MemoryTransaction* activeTransaction) {
    if (m_cacheView) {
        m_cacheView->sync(cache, ram, activeTransaction);
    }
}

CacheView* CpuView::getPrimaryCacheView() {
    return m_cacheView.get();
}

const CacheView* CpuView::getPrimaryCacheView() const {
    return m_cacheView.get();
}

void CpuView::rebuildUnits() {
    if (!m_cacheView) {
        m_cacheView = std::make_unique<CacheView>(getFont());
        addChild(*m_cacheView);
    }
    if (!m_aluView) {
        m_aluView = std::make_unique<AluView>(getFont());
        addChild(*m_aluView);
    }
}

void CpuView::layoutBlock() {
    BlockView::layoutBlock();
    rebuildUnits();

    const sf::Vector2f size = getBlockSize();

    if (PortView* cacheIn = findPort("cache_in")) {
        cacheIn->setLocalAnchor({size.x, size.y * 0.5f});
        cacheIn->setSize({kIoPortWidth, kIoPortHeight});
        cacheIn->setColors(kTransparentPortColor, kTransparentPortColor);
    }

    const float contentWidth = size.x - kContentPadding * 2.0f;
    const float contentHeight = size.y - kTopContentY - kContentPadding;
    const float aluWidth = contentWidth * 0.42f;
    const float cacheWidth = contentWidth - aluWidth - kContentGap;
    const sf::Vector2f aluPosition{kContentPadding, kTopContentY};
    const sf::Vector2f aluSize{aluWidth, contentHeight};
    const sf::Vector2f cachePosition{kContentPadding + aluWidth + kContentGap, kTopContentY};
    const sf::Vector2f cacheSize{cacheWidth, contentHeight};

    if (m_aluView) {
        m_aluView->setPosition(aluPosition);
        m_aluView->setViewSize(aluSize);
    }

    if (m_cacheView) {
        m_cacheView->setPosition(cachePosition);
        m_cacheView->setViewSize(cacheSize);
    }
}

void CpuView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {}
} // namespace view
