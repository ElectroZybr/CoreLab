#include "view/CpuView.h"

#include <numbers>

namespace {
constexpr float kContentPadding = 42.0f;
constexpr float kTopContentY = 170.0f;
constexpr float kContentGap = 26.0f;
constexpr float kBottomPadding = 42.0f;
constexpr float kAluPreferredWidth = 1360.0f;
constexpr float kAluMinimumHeight = 1280.0f;
constexpr float kIoPortWidth = 52.0f;
constexpr float kIoPortHeight = 28.0f;
constexpr float kCacheToAluTurnRadius = 90.0f;
constexpr float kCacheToAluTopRise = 56.0f;
const sf::Color kTransparentPortColor(0, 0, 0, 0);
const sf::Color kInternalRailColor(116, 134, 165);
} // namespace

namespace view {
CpuView::CpuView(const sf::Font* font, sf::Vector2f position)
    : BlockView(font, position, {0.0f, 0.0f}) {
    setHeaderLayout({88.0f, 20.0f, 106.0f, 82, 20});
    setTitle("CPU");
    setSubtitle("L1 cache + execution pipeline");
    addPort("cache_in", PortKind::Input, PortDirection::Right, PayloadKind::CacheLine);
    rebuildUnits();
    updateSizeFromContent();
    layoutBlock();
}

void CpuView::syncPrimaryCache(const sim::Cache& cache,
                               const sim::RAM* ram,
                               const sim::MemoryTransaction* activeTransaction) {
    if (m_cacheView) {
        m_cacheView->sync(cache, ram, activeTransaction);
        updateSizeFromContent();
    }
}

CacheView* CpuView::getPrimaryCacheView() {
    return m_cacheView.get();
}

const CacheView* CpuView::getPrimaryCacheView() const {
    return m_cacheView.get();
}

void CpuView::setRegisterLabels(const std::array<std::string, CacheLineView::kFloatCount>& labels) {
    if (m_aluView) {
        m_aluView->setRegisterLabels(labels);
    }
}

sf::Vector2f CpuView::getRegisterCellCenter(std::size_t index) const {
    if (!m_aluView) {
        return getWorldPosition();
    }

    return m_aluView->getRegisterCellCenter(index);
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

void CpuView::updateSizeFromContent() {
    rebuildUnits();

    const sf::Vector2f cacheSize = m_cacheView ? m_cacheView->getViewSize() : sf::Vector2f{0.0f, 0.0f};
    const float contentHeight = std::max(cacheSize.y, kAluMinimumHeight);
    const float width = kContentPadding * 2.0f + kAluPreferredWidth + kContentGap + cacheSize.x;
    const float height = kTopContentY + contentHeight + kBottomPadding;
    setBlockSize({width, height});
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
    const float cacheWidth = m_cacheView ? m_cacheView->getViewSize().x : 0.0f;
    const float aluWidth = contentWidth - cacheWidth - kContentGap;
    const sf::Vector2f aluPosition{kContentPadding, kTopContentY};
    const sf::Vector2f aluSize{aluWidth, contentHeight};
    const sf::Vector2f cachePosition{
        size.x - kContentPadding - cacheWidth,
        kTopContentY,
    };
    const sf::Vector2f cacheSize{cacheWidth, contentHeight};

    if (m_aluView) {
        m_aluView->setPosition(aluPosition);
        m_aluView->setViewSize(aluSize);
    }

    if (m_cacheView) {
        m_cacheView->setPosition(cachePosition);
        m_cacheView->setViewSize(cacheSize);
    }

    if (m_cacheView && m_aluView) {
        const PortView* cacheOut = m_cacheView->findPort("cpu_out");
        const PortView* zmmIn = m_aluView->findPort("zmm_in");
        if (cacheOut && zmmIn) {
            const rails::RailStyle style{6.0f, kInternalRailColor};
            m_cacheToAluPath = rails::RailPath(style);

            const sf::Vector2f start = cacheOut->getWorldAnchor();
            const sf::Vector2f end = zmmIn->getWorldAnchor();
            const float radius = kCacheToAluTurnRadius;
            const float topY = start.y - radius;
            const float verticalX = end.x + radius;
            const float verticalEndY = end.y - radius;

            m_cacheToAluPath.appendArc({start.x - radius, start.y},
                                       radius,
                                       0.0f,
                                       -std::numbers::pi_v<float> * 0.5f);

            const sf::Vector2f topStart{start.x - radius, topY};
            const sf::Vector2f topEnd{verticalX + radius, topY};
            if (std::abs(topEnd.x - topStart.x) > 0.5f) {
                m_cacheToAluPath.appendStraight(topStart, topEnd);
            }

            m_cacheToAluPath.appendArc({verticalX + radius, topY + radius},
                                       radius,
                                       -std::numbers::pi_v<float> * 0.5f,
                                       -std::numbers::pi_v<float>);

            const sf::Vector2f downStart{verticalX, topY + radius};
            const sf::Vector2f downEnd{verticalX, verticalEndY};
            if (downStart.y < downEnd.y - 0.5f) {
                m_cacheToAluPath.appendStraight(downStart, downEnd);
            }

            m_cacheToAluPath.appendArc({verticalX - radius, end.y - radius},
                                       radius,
                                       0.0f,
                                       std::numbers::pi_v<float> * 0.5f);

            const sf::Vector2f entryStart{verticalX - radius, end.y};
            if (std::abs(entryStart.x - end.x) > 0.5f) {
                m_cacheToAluPath.appendStraight(entryStart, end);
            }
        } else {
            m_cacheToAluPath = {};
        }
    } else {
        m_cacheToAluPath = {};
    }
}

void CpuView::drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const {
}

void CpuView::drawBlockOverlay(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_cacheToAluPath.isEmpty()) {
        target.draw(m_cacheToAluPath, states);
    }
}
} // namespace view
