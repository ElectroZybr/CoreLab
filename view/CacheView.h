#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <vector>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "view/CacheLineView.h"

namespace view {
class CacheView : public sf::Drawable {
  public:
    explicit CacheView(const sf::Font* font = nullptr, sf::Vector2f position = {0.0f, 0.0f});

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const {
        return m_position;
    }
    [[nodiscard]] sf::FloatRect getBounds() const;
    [[nodiscard]] bool isInDragHandle(sf::Vector2f worldPoint) const;
    void setDragState(bool hovered, bool dragging);
    [[nodiscard]] sf::Vector2f getLinePosition() const;
    [[nodiscard]] sf::Vector2f getEntryPosition() const;

    void setFont(const sf::Font* font);
    void sync(const sim::Cache& cache, const sim::MemoryTransaction* activeTransaction = nullptr);

  private:
    void rebuildContainer();
    void rebuildSlots(std::size_t slotCount);
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::Vector2f m_size{0.0f, 0.0f};
    std::size_t m_cacheSizeInBytes = 0;
    std::size_t m_selectedSlotIndex = 0;
    bool m_selectedSlotValid = false;

    sf::ConvexShape m_container;
    sf::ConvexShape m_inputPort;
    sf::ConvexShape m_dragHandleOverlay;
    std::vector<sf::RectangleShape> m_dragHandleMarks;
    bool m_dragHovered = false;
    bool m_dragging = false;
    sf::ConvexShape m_selectionFrame;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_summaryText;
    std::vector<CacheLineView> m_slotViews;
    std::vector<sf::ConvexShape> m_slotOverlays;
};
} // namespace view
