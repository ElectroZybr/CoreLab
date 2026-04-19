#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "view/BlockView.h"
#include "view/CacheView.h"

namespace view {
class CoreView : public BlockView {
  public:
    explicit CoreView(std::string title = "Core",
                      bool active = false,
                      const sf::Font* font = nullptr,
                      sf::Vector2f position = {0.0f, 0.0f},
                      sf::Vector2f size = {0.0f, 0.0f});

    void setCoreSize(sf::Vector2f size);
    void syncCache(const sim::Cache& cache, const sim::MemoryTransaction* activeTransaction = nullptr);
    [[nodiscard]] bool hasCache() const {
        return m_cacheView != nullptr;
    }
    [[nodiscard]] CacheView* getCacheView() {
        return m_cacheView.get();
    }
    [[nodiscard]] const CacheView* getCacheView() const {
        return m_cacheView.get();
    }

  private:
    enum class ModuleType {
        Control,
        Simd,
        Alu,
        Registers,
    };

    void rebuildModules();
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;

    bool m_active = false;
    std::unique_ptr<CacheView> m_cacheView;
    std::array<sf::ConvexShape, 4> m_modules;
    std::array<std::optional<sf::Text>, 4> m_moduleTexts;
};
} // namespace view
