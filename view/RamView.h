#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "view/CacheLineView.h"
#include "view/rails/RailPath.h"

namespace view {
class RamView : public sf::Drawable {
  public:
    static constexpr std::size_t kCacheLineSizeInBytes = 64;

    struct ReadPath {
        sf::Vector2f sourcePosition;
        sf::Vector2f lanePosition;
        sf::Vector2f turnEntryPosition;
        sf::Vector2f turnCenter;
        float turnRadius = 0.0f;
        float turnStartAngle = 0.0f;
        float turnEndAngle = 0.0f;
        sf::Vector2f turnExitPosition;
        sf::Vector2f collectorPosition;
        sf::Vector2f junctionTurnCenter;
        float junctionTurnRadius = 0.0f;
        float junctionTurnStartAngle = 0.0f;
        float junctionTurnEndAngle = 0.0f;
        sf::Vector2f junctionTurnExitPosition;
        sf::Vector2f exitPosition;
    };

    explicit RamView(std::size_t sizeInBytes,
                     const sf::Font* font = nullptr,
                     sf::Vector2f position = {0.0f, 0.0f});

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const {
        return m_position;
    }
    [[nodiscard]] sf::FloatRect getBounds() const;
    [[nodiscard]] bool isInDragHandle(sf::Vector2f worldPoint) const;
    void setDragState(bool hovered, bool dragging);
    [[nodiscard]] sf::Vector2f getLinePosition(std::size_t index) const;
    [[nodiscard]] sf::Vector2f getLineHeadCenter(std::size_t index) const;
    [[nodiscard]] ReadPath getReadPath(std::size_t index) const;
    void setHighlightedLine(std::optional<std::size_t> lineIndex);
    [[nodiscard]] std::size_t getSizeInBytes() const {
        return m_sizeInBytes;
    }
    [[nodiscard]] std::size_t getSlotCount() const {
        return m_slotCount;
    }

    void setFont(const sf::Font* font);

  private:
    void rebuildGeometry();
    void rebuildText();
    void layout();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    std::size_t m_sizeInBytes = 0;
    std::size_t m_slotCount = 0;
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::Vector2f m_size{0.0f, 0.0f};
    sf::ConvexShape m_container;
    sf::ConvexShape m_outputPort;
    sf::ConvexShape m_dragHandleOverlay;
    std::vector<sf::RectangleShape> m_dragHandleMarks;
    bool m_dragHovered = false;
    bool m_dragging = false;
    std::vector<rails::RailPath> m_railPaths;
    rails::RailPath m_highlightPath;
    std::optional<std::size_t> m_highlightedLineIndex;
    std::vector<CacheLineView> m_lines;
    std::optional<sf::Text> m_titleText;
};
} // namespace view
