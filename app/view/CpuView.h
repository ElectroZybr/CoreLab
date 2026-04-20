#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <array>
#include <string>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "view/AluView.h"
#include "view/BlockView.h"
#include "view/CacheView.h"
#include "view/rails/RailPath.h"

namespace view {
class CpuView : public BlockView {
  public:
    explicit CpuView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    void syncPrimaryCache(const sim::Cache& cache,
                          const sim::RAM* ram = nullptr,
                          const sim::MemoryTransaction* activeTransaction = nullptr);
    [[nodiscard]] CacheView* getPrimaryCacheView();
    [[nodiscard]] const CacheView* getPrimaryCacheView() const;
    void setRegisterLabels(const std::array<std::string, CacheLineView::kFloatCount>& labels);
    [[nodiscard]] sf::Vector2f getRegisterCellCenter(std::size_t index) const;
    [[nodiscard]] const rails::RailPath& getCacheToAluPath() const {
        return m_cacheToAluPath;
    }

  private:
    void rebuildUnits();
    void updateSizeFromContent();
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;
    void drawBlockOverlay(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::unique_ptr<CacheView> m_cacheView;
    std::unique_ptr<AluView> m_aluView;
    rails::RailPath m_cacheToAluPath;
};
} // namespace view
