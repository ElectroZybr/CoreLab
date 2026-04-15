#include "animation/MemoryReadAnimation.h"

#include <cmath>
#include <numbers>

namespace {
float length(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

sf::Vector2f toTopLeft(sf::Vector2f center) {
    return {center.x - view::CacheLineView::kWidth * 0.5f, center.y - view::CacheLineView::kHeight * 0.5f};
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
                                   float turnStartAngle,
                                   float turnEndAngle,
                                   sf::Vector2f turnExitPosition,
                                   sf::Vector2f collectorPosition,
                                   sf::Vector2f junctionTurnCenter,
                                   float junctionTurnRadius,
                                   float junctionTurnStartAngle,
                                   float junctionTurnEndAngle,
                                   sf::Vector2f junctionTurnExitPosition,
                                   sf::Vector2f exitPosition,
                                   sf::Vector2f targetPosition) {
    m_sourcePosition = sourcePosition;
    m_lanePosition = lanePosition;
    m_turnEntryPosition = turnEntryPosition;
    m_turnCenter = turnCenter;
    m_turnRadius = turnRadius;
    m_turnStartAngle = turnStartAngle;
    m_turnEndAngle = turnEndAngle;
    m_turnExitPosition = turnExitPosition;
    m_collectorPosition = collectorPosition;
    m_junctionTurnCenter = junctionTurnCenter;
    m_junctionTurnRadius = junctionTurnRadius;
    m_junctionTurnStartAngle = junctionTurnStartAngle;
    m_junctionTurnEndAngle = junctionTurnEndAngle;
    m_junctionTurnExitPosition = junctionTurnExitPosition;
    m_exitPosition = exitPosition;
    m_targetPosition = targetPosition;
    m_copy.setPosition(sourcePosition);
    m_hasRoute = true;
    m_visible = false;
}

void MemoryReadAnimation::sync(
    const sim::MemoryTransaction& transaction, sim::Tick tick, const view::rails::RailPath* busPath) {
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
        if (busPath && !busPath->isEmpty() && busPath->getLength() > 0.0f) {
            const float distance = busPath->getLength() * easeInOut(transaction.getPhaseProgress(tick));
            m_copy.setPosition(toTopLeft(busPath->samplePoint(distance)));
        } else {
            m_copy.setPosition(
                lerp(m_exitPosition, m_targetPosition, easeInOut(transaction.getPhaseProgress(tick))));
        }
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

sf::Vector2f MemoryReadAnimation::sampleArcPosition(
    sf::Vector2f center, float radius, float startAngle, float endAngle, float t) {
    const float angle = startAngle + (endAngle - startAngle) * softEase(t);
    const sf::Vector2f shapeCenter{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
    return {shapeCenter.x - view::CacheLineView::kWidth * 0.5f,
            shapeCenter.y - view::CacheLineView::kHeight * 0.5f};
}

sf::Vector2f MemoryReadAnimation::sampleToRamPort(float t) const {
    const float laneDistance = length(m_lanePosition - m_sourcePosition);
    const float toTurnEntryDistance = length(m_turnEntryPosition - m_lanePosition);
    const float firstTurnDistance = std::abs(m_turnEndAngle - m_turnStartAngle) * m_turnRadius;
    const float collectorDistance = length(m_collectorPosition - m_turnExitPosition);
    const float junctionTurnDistance =
        std::abs(m_junctionTurnEndAngle - m_junctionTurnStartAngle) * m_junctionTurnRadius;
    const float exitDistance = length(m_exitPosition - m_junctionTurnExitPosition);
    const float totalDistance = laneDistance + toTurnEntryDistance + firstTurnDistance + collectorDistance +
                                junctionTurnDistance + exitDistance;

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

    if (remainingDistance <= firstTurnDistance) {
        return sampleArcPosition(m_turnCenter,
                                 m_turnRadius,
                                 m_turnStartAngle,
                                 m_turnEndAngle,
                                 firstTurnDistance > 0.0f ? remainingDistance / firstTurnDistance : 1.0f);
    }
    remainingDistance -= firstTurnDistance;

    if (remainingDistance <= collectorDistance) {
        return lerp(m_turnExitPosition,
                    m_collectorPosition,
                    collectorDistance > 0.0f ? remainingDistance / collectorDistance : 1.0f);
    }
    remainingDistance -= collectorDistance;

    if (remainingDistance <= junctionTurnDistance) {
        return sampleArcPosition(m_junctionTurnCenter,
                                 m_junctionTurnRadius,
                                 m_junctionTurnStartAngle,
                                 m_junctionTurnEndAngle,
                                 junctionTurnDistance > 0.0f ? remainingDistance / junctionTurnDistance
                                                             : 1.0f);
    }
    remainingDistance -= junctionTurnDistance;

    return lerp(m_junctionTurnExitPosition,
                m_exitPosition,
                exitDistance > 0.0f ? remainingDistance / exitDistance : 1.0f);
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_hasRoute || !m_visible) {
        return;
    }

    target.draw(m_copy, states);
}
