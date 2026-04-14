#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>

#include "objects/RAM.h"

namespace view {
class RamView : public sf::Drawable {
  public:
    static constexpr std::size_t kCacheLineSizeInBytes = RAM::kCacheLineSizeInBytes;
    using ReadPath = RAM::ReadPath;

    explicit RamView(std::size_t sizeInBytes, const sf::Font* font = nullptr);

    void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const;
    [[nodiscard]] sf::Vector2f getLinePosition(std::size_t index) const;
    [[nodiscard]] ReadPath getReadPath(std::size_t index) const;
    [[nodiscard]] std::size_t getSizeInBytes() const;
    [[nodiscard]] std::size_t getSlotCount() const;

    void setFont(const sf::Font* font);

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    RAM m_ram;
};
} // namespace view
