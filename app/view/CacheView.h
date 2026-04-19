#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <vector>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "sim/RAM.h"
#include "view/BlockView.h"
#include "view/CacheLineView.h"
#include "view/rails/RailPath.h"
#include "view/rails/RailBuilder.h"

namespace view {
class CacheView : public BlockView {
  public:
    explicit CacheView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    [[nodiscard]] sf::Vector2f getLinePosition() const;
    [[nodiscard]] sf::Vector2f getLinePosition(std::size_t slotIndex) const;
    [[nodiscard]] sf::Vector2f getEntryPosition() const;
    [[nodiscard]] sf::Vector2f getLineHeadCenter() const;
    [[nodiscard]] sf::Vector2f getLineHeadCenter(std::size_t slotIndex) const;
    [[nodiscard]] sf::Vector2f getEntryCenter() const;
    [[nodiscard]] rails::RailDirection getEntryDirection() const {
        return rails::RailDirection::Down;
    }
    [[nodiscard]] const rails::RailPath& getInstallPath() const {
        return getInstallPath(m_selectedSlotIndex);
    }
    [[nodiscard]] const rails::RailPath& getInstallPath(std::size_t slotIndex) const {
        if (m_installPaths.empty()) {
            static const rails::RailPath kEmptyPath;
            return kEmptyPath;
        }

        return m_installPaths[std::min(slotIndex, m_installPaths.size() - 1)];
    }
    void setHighlightedSlot(std::optional<std::size_t> slotIndex);

    void sync(const sim::Cache& cache,
              const sim::RAM* ram = nullptr,
              const sim::MemoryTransaction* activeTransaction = nullptr);

  private:
    void rebuildContainer();
    void rebuildSlots(std::size_t slotCount);
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::size_t m_cacheSizeInBytes = 0;
    std::size_t m_selectedSlotIndex = 0;
    bool m_selectedSlotValid = false;

    sf::ConvexShape m_inputPort;
    std::vector<rails::RailPath> m_railPaths;
    std::vector<rails::RailPath> m_installPaths;
    rails::RailPath m_highlightPath;
    std::optional<std::size_t> m_highlightedSlotIndex;
    std::vector<CacheLineView> m_slotViews;
    std::vector<sf::ConvexShape> m_slotOverlays;
};
} // namespace view
