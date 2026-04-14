#pragma once

#include <SFML/Graphics.hpp>

#include "objects/CacheLine.h"
#include "sim/MemoryTransaction.h"

class MemoryReadAnimation : public sf::Drawable {
  public:
    explicit MemoryReadAnimation(const sf::Font* font = nullptr);

    void setFont(const sf::Font* font);
    void setRoute(sf::Vector2f sourcePosition,
                  sf::Vector2f lanePosition,
                  sf::Vector2f turnEntryPosition,
                  sf::Vector2f turnCenter,
                  float turnRadius,
                  sf::Vector2f turnExitPosition,
                  sf::Vector2f exitPosition,
                  sf::Vector2f targetPosition);
    void sync(const sim::MemoryTransaction& transaction, sim::Tick tick);
    void clear();

  private:
    [[nodiscard]] static sf::Vector2f lerp(sf::Vector2f from, sf::Vector2f to, float t);
    [[nodiscard]] static float easeInOut(float t);
    [[nodiscard]] static float softEase(float t);
    [[nodiscard]] static sf::Vector2f sampleTurnPosition(sf::Vector2f center, float radius, float t);
    [[nodiscard]] sf::Vector2f sampleToRamPort(float t) const;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    CacheLine m_copy;
    sf::Vector2f m_sourcePosition{0.0f, 0.0f};
    sf::Vector2f m_lanePosition{0.0f, 0.0f};
    sf::Vector2f m_turnEntryPosition{0.0f, 0.0f};
    sf::Vector2f m_turnCenter{0.0f, 0.0f};
    float m_turnRadius = 0.0f;
    sf::Vector2f m_turnExitPosition{0.0f, 0.0f};
    sf::Vector2f m_exitPosition{0.0f, 0.0f};
    sf::Vector2f m_targetPosition{0.0f, 0.0f};
    bool m_hasRoute = false;
    bool m_visible = false;
};
