#include "animation/MemoryReadAnimation.h"

#include <algorithm>

namespace
{
constexpr float kPauseDuration = 0.9f;
constexpr float kExitDuration = 0.4f;
constexpr float kToBusDuration = 0.75f;
constexpr float kToExitDuration = 0.45f;
constexpr float kTravelDuration = 1.0f;
constexpr float kHoldDuration = 0.55f;
}

MemoryReadAnimation::MemoryReadAnimation(const sf::Font* font) : m_font(font), m_copy(font)
{
}

void MemoryReadAnimation::setFont(const sf::Font* font)
{
    m_font = font;
    m_copy.setFont(font);
}

void MemoryReadAnimation::setRoute(
    sf::Vector2f sourcePosition,
    sf::Vector2f lanePosition,
    sf::Vector2f busPosition,
    sf::Vector2f exitPosition,
    sf::Vector2f targetPosition
)
{
    m_sourcePosition = sourcePosition;
    m_lanePosition = lanePosition;
    m_busPosition = busPosition;
    m_exitPosition = exitPosition;
    m_targetPosition = targetPosition;
    m_copy.setPosition(sourcePosition);
    m_phaseTime = 0.0f;
    m_phase = Phase::Pause;
    m_hasRoute = true;
}

void MemoryReadAnimation::update(float deltaSeconds)
{
    if (!m_hasRoute)
    {
        return;
    }

    m_phaseTime += deltaSeconds;

    switch (m_phase)
    {
    case Phase::Pause:
        if (m_phaseTime >= kPauseDuration)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::Exit;
            m_copy.setPosition(m_sourcePosition);
        }
        break;

    case Phase::Exit:
    {
        const float t = std::min(m_phaseTime / kExitDuration, 1.0f);
        m_copy.setPosition(lerp(m_sourcePosition, m_lanePosition, easeInOut(t)));

        if (t >= 1.0f)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::ToBus;
        }
        break;
    }

    case Phase::ToBus:
    {
        const float t = std::min(m_phaseTime / kToBusDuration, 1.0f);
        m_copy.setPosition(lerp(m_lanePosition, m_busPosition, easeInOut(t)));

        if (t >= 1.0f)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::ToExit;
        }
        break;
    }

    case Phase::ToExit:
    {
        const float t = std::min(m_phaseTime / kToExitDuration, 1.0f);
        m_copy.setPosition(lerp(m_busPosition, m_exitPosition, easeInOut(t)));

        if (t >= 1.0f)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::Travel;
        }
        break;
    }

    case Phase::Travel:
    {
        const float t = std::min(m_phaseTime / kTravelDuration, 1.0f);
        m_copy.setPosition(lerp(m_exitPosition, m_targetPosition, easeInOut(t)));

        if (t >= 1.0f)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::Hold;
        }
        break;
    }

    case Phase::Hold:
        m_copy.setPosition(m_targetPosition);
        if (m_phaseTime >= kHoldDuration)
        {
            m_phaseTime = 0.0f;
            m_phase = Phase::Pause;
        }
        break;
    }
}

sf::Vector2f MemoryReadAnimation::lerp(sf::Vector2f from, sf::Vector2f to, float t)
{
    return from + (to - from) * t;
}

float MemoryReadAnimation::easeInOut(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (!m_hasRoute || m_phase == Phase::Pause)
    {
        return;
    }

    target.draw(m_copy, states);
}
