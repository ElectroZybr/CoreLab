#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "sim/RAM.h"
#include "view/BlockView.h"
#include "view/CacheLineView.h"
#include "view/rails/RailPath.h"

namespace view {
class RamView : public BlockView {
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

    [[nodiscard]] sf::Vector2f getLinePosition(std::size_t index) const;
    [[nodiscard]] sf::Vector2f getLineHeadCenter(std::size_t index) const;
    [[nodiscard]] ReadPath getReadPath(std::size_t index) const;
    [[nodiscard]] sim::RAM::LineCellLabels getLineLabels(std::size_t index) const;
    void sync(const sim::RAM& ram);
    void setHighlightedLine(std::optional<std::size_t> lineIndex);
    [[nodiscard]] std::size_t getSizeInBytes() const {
        return m_sizeInBytes;
    }
    [[nodiscard]] std::size_t getSlotCount() const {
        return m_slotCount;
    }

  private:
    void rebuildGeometry();
    void layoutBlock() override;
    void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::size_t m_sizeInBytes = 0;
    std::size_t m_slotCount = 0;
    sf::ConvexShape m_outputPort;
    std::vector<rails::RailPath> m_railPaths;
    rails::RailPath m_highlightPath;
    std::optional<std::size_t> m_highlightedLineIndex;
    std::vector<CacheLineView> m_lines;
};
} // namespace view
