#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "view/BlockView.h"
#include "view/CoreView.h"

namespace view {
class CpuView : public BlockView {
  public:
    explicit CpuView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    void syncPrimaryCache(const sim::Cache& cache, const sim::MemoryTransaction* activeTransaction = nullptr);
    [[nodiscard]] CoreView* getPrimaryCore() {
        return m_cores.empty() ? nullptr : m_cores.front().get();
    }
    [[nodiscard]] const CoreView* getPrimaryCore() const {
        return m_cores.empty() ? nullptr : m_cores.front().get();
    }
    [[nodiscard]] CacheView* getPrimaryCacheView();
    [[nodiscard]] const CacheView* getPrimaryCacheView() const;

  private:
    void rebuildCores();
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::vector<std::unique_ptr<CoreView>> m_cores;
};
} // namespace view
