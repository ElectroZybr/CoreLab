#include "animation/MemoryReadAnimation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
constexpr float kPauseDuration = 0.9f;
constexpr float kExitDuration = 0.28f;
constexpr float kTravelDuration = 0.15f;
constexpr float kHoldDuration = 0.55f;
constexpr float kInternalRouteSpeed = 5000.0f;
constexpr float kMinimumPhaseDuration = 0.001f;
constexpr float kTurnStartAngle = std::numbers::pi_v<float> * 0.5f;
constexpr float kTurnEndAngle = std::numbers::pi_v<float>;

float length(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

float durationForDistance(float distance, float speed) {
    return std::max(distance / speed, kMinimumPhaseDuration);
}
} // namespace

MemoryReadAnimation::MemoryReadAnimation(const sf::Font* font) : m_font(font), m_copy(font) {
}

void MemoryReadAnimation::setFont(const sf::Font* font) {
    m_font = font;
    m_copy.setFont(font);
}

void MemoryReadAnimation::setRoute(sf::Vector2f sourcePosition,
                                   sf::Vector2f lanePosition,
                                   sf::Vector2f turnEntryPosition,
                                   sf::Vector2f turnCenter,
                                   float turnRadius,
                                   sf::Vector2f turnExitPosition,
                                   sf::Vector2f exitPosition,
                                   sf::Vector2f targetPosition) {
    m_sourcePosition = sourcePosition;
    m_lanePosition = lanePosition;
    m_turnEntryPosition = turnEntryPosition;
    m_turnCenter = turnCenter;
    m_turnRadius = turnRadius;
    m_turnExitPosition = turnExitPosition;
    m_exitPosition = exitPosition;
    m_targetPosition = targetPosition;
    m_toTurnEntryDuration =
        durationForDistance(length(m_turnEntryPosition - m_lanePosition), kInternalRouteSpeed);
    m_turnDuration =
        durationForDistance((kTurnEndAngle - kTurnStartAngle) * m_turnRadius, kInternalRouteSpeed);
    m_toExitDuration = durationForDistance(length(m_exitPosition - m_turnExitPosition), kInternalRouteSpeed);
    m_copy.setPosition(sourcePosition);
    m_phaseTime = 0.0f;
    m_phase = Phase::Pause;
    m_hasRoute = true;
}

void MemoryReadAnimation::update(float deltaSeconds) {
    if (!m_hasRoute) {
        return;
    }

    m_phaseTime += deltaSeconds;

    switch (m_phase) {
    case Phase::Pause:
        if (m_phaseTime >= kPauseDuration) {
            m_phaseTime = 0.0f;
            m_phase = Phase::Exit;
            m_copy.setPosition(m_sourcePosition);
        }
        break;

    case Phase::Exit: {
        const float t = std::min(m_phaseTime / kExitDuration, 1.0f);
        m_copy.setPosition(lerp(m_sourcePosition, m_lanePosition, easeInOut(t)));

        if (t >= 1.0f) {
            m_phaseTime = 0.0f;
            m_phase = Phase::ToTurnEntry;
        }
        break;
    }

    case Phase::ToTurnEntry: {
        const float t = std::min(m_phaseTime / m_toTurnEntryDuration, 1.0f);
        m_copy.setPosition(lerp(m_lanePosition, m_turnEntryPosition, softEase(t)));

        if (t >= 1.0f) {
            m_phaseTime = 0.0f;
            m_phase = Phase::Turn;
        }
        break;
    }

    case Phase::Turn: {
        const float t = std::min(m_phaseTime / m_turnDuration, 1.0f);
        const float angle = kTurnStartAngle + (kTurnEndAngle - kTurnStartAngle) * softEase(t);
        const sf::Vector2f center{m_turnCenter.x + std::cos(angle) * m_turnRadius,
                                  m_turnCenter.y + std::sin(angle) * m_turnRadius};
        m_copy.setPosition({center.x - CacheLine::kWidth * 0.5f, center.y - CacheLine::kHeight * 0.5f});

        if (t >= 1.0f) {
            m_phaseTime = 0.0f;
            m_phase = Phase::ToExit;
        }
        break;
    }

    case Phase::ToExit: {
        const float t = std::min(m_phaseTime / m_toExitDuration, 1.0f);
        m_copy.setPosition(lerp(m_turnExitPosition, m_exitPosition, softEase(t)));

        if (t >= 1.0f) {
            m_phaseTime = 0.0f;
            m_phase = Phase::Travel;
        }
        break;
    }

    case Phase::Travel: {
        const float t = std::min(m_phaseTime / kTravelDuration, 1.0f);
        m_copy.setPosition(lerp(m_exitPosition, m_targetPosition, easeInOut(t)));

        if (t >= 1.0f) {
            m_phaseTime = 0.0f;
            m_phase = Phase::Hold;
        }
        break;
    }

    case Phase::Hold:
        m_copy.setPosition(m_targetPosition);
        if (m_phaseTime >= kHoldDuration) {
            m_phaseTime = 0.0f;
            m_phase = Phase::Pause;
            m_cycleCompleted = true;
        }
        break;
    }
}

bool MemoryReadAnimation::consumeCycleCompleted() {
    const bool completed = m_cycleCompleted;
    m_cycleCompleted = false;
    return completed;
}

sf::Vector2f MemoryReadAnimation::lerp(sf::Vector2f from, sf::Vector2f to, float t) {
    return from + (to - from) * t;
}

float MemoryReadAnimation::easeInOut(float t) {
    return 0.5f - 0.5f * std::cos(std::numbers::pi_v<float> * t);
}

float MemoryReadAnimation::softEase(float t) {
    constexpr float amplitude = 0.22f;
    return t -
           amplitude * std::sin(std::numbers::pi_v<float> * 2.0f * t) / (std::numbers::pi_v<float> * 2.0f);
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_hasRoute || m_phase == Phase::Pause || m_phase == Phase::Hold) {
        return;
    }

    target.draw(m_copy, states);
}
