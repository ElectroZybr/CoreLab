#pragma once

#include <SFML/Graphics.hpp>

#include "objects/CacheLine.h"

class MemoryReadAnimation : public sf::Drawable
{
public:
    explicit MemoryReadAnimation(const sf::Font* font = nullptr);

    void setFont(const sf::Font* font);
    void setRoute(
        sf::Vector2f sourcePosition,
        sf::Vector2f lanePosition,
        sf::Vector2f turnEntryPosition,
        sf::Vector2f turnCenter,
        float turnRadius,
        sf::Vector2f turnExitPosition,
        sf::Vector2f exitPosition,
        sf::Vector2f targetPosition
    );
    void update(float deltaSeconds);
    [[nodiscard]] bool consumeCycleCompleted();

private:
    enum class Phase
    {
        Pause,
        Exit,
        ToTurnEntry,
        Turn,
        ToExit,
        Travel,
        Hold
    };

    [[nodiscard]] static sf::Vector2f lerp(sf::Vector2f from, sf::Vector2f to, float t);
    [[nodiscard]] static float easeInOut(float t);
    [[nodiscard]] static float softEase(float t);
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
    float m_toTurnEntryDuration = 0.0f;
    float m_turnDuration = 0.0f;
    float m_toExitDuration = 0.0f;
    float m_phaseTime = 0.0f;
    Phase m_phase = Phase::Pause;
    bool m_hasRoute = false;
    bool m_cycleCompleted = false;
};
