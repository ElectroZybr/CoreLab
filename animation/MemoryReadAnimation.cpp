#include "animation/MemoryReadAnimation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
constexpr float kTurnStartAngle = std::numbers::pi_v<float> * 0.5f;
constexpr float kTurnEndAngle = std::numbers::pi_v<float>;

float length(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
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
    m_copy.setPosition(sourcePosition);
    m_hasRoute = true;
    m_visible = false;
}

void MemoryReadAnimation::sync(const sim::MemoryTransaction& transaction, sim::Tick tick) {
    if (!m_hasRoute || transaction.isCompleted(tick)) {
        m_visible = false;
        return;
    }

    switch (transaction.getPhase(tick)) {
    case sim::MemoryTransactionPhase::ToRamPort:
        m_copy.setPosition(sampleToRamPort(transaction.getPhaseProgress(tick)));
        m_visible = true;
        break;

    case sim::MemoryTransactionPhase::OnBus:
        m_copy.setPosition(
            lerp(m_exitPosition, m_targetPosition, easeInOut(transaction.getPhaseProgress(tick))));
        m_visible = true;
        break;

    case sim::MemoryTransactionPhase::Install:
        m_copy.setPosition(m_targetPosition);
        m_visible = true;
        break;

    case sim::MemoryTransactionPhase::Completed:
        m_visible = false;
        break;
    }
}

void MemoryReadAnimation::clear() {
    m_visible = false;
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

sf::Vector2f MemoryReadAnimation::sampleTurnPosition(sf::Vector2f center, float radius, float t) {
    const float angle = kTurnStartAngle + (kTurnEndAngle - kTurnStartAngle) * softEase(t);
    const sf::Vector2f shapeCenter{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
    return {shapeCenter.x - CacheLine::kWidth * 0.5f, shapeCenter.y - CacheLine::kHeight * 0.5f};
}

sf::Vector2f MemoryReadAnimation::sampleToRamPort(float t) const {
    const float laneDistance = length(m_lanePosition - m_sourcePosition);
    const float toTurnEntryDistance = length(m_turnEntryPosition - m_lanePosition);
    const float turnDistance = (kTurnEndAngle - kTurnStartAngle) * m_turnRadius;
    const float toExitDistance = length(m_exitPosition - m_turnExitPosition);
    const float totalDistance = laneDistance + toTurnEntryDistance + turnDistance + toExitDistance;

    if (totalDistance <= 0.0f) {
        return m_sourcePosition;
    }

    float remainingDistance = totalDistance * softEase(t);

    if (remainingDistance <= laneDistance) {
        return lerp(
            m_sourcePosition, m_lanePosition, laneDistance > 0.0f ? remainingDistance / laneDistance : 1.0f);
    }
    remainingDistance -= laneDistance;

    if (remainingDistance <= toTurnEntryDistance) {
        return lerp(m_lanePosition,
                    m_turnEntryPosition,
                    toTurnEntryDistance > 0.0f ? remainingDistance / toTurnEntryDistance : 1.0f);
    }
    remainingDistance -= toTurnEntryDistance;

    if (remainingDistance <= turnDistance) {
        return sampleTurnPosition(
            m_turnCenter, m_turnRadius, turnDistance > 0.0f ? remainingDistance / turnDistance : 1.0f);
    }
    remainingDistance -= turnDistance;

    return lerp(m_turnExitPosition,
                m_exitPosition,
                toExitDistance > 0.0f ? remainingDistance / toExitDistance : 1.0f);
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_hasRoute || !m_visible) {
        return;
    }

    target.draw(m_copy, states);
}
