#include "animation/MemoryReadAnimation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
constexpr std::size_t kCurveSampleCount = 28;
constexpr float kAnimatedTrainWidth = 82.0f;
constexpr float kAnimatedTrainLength = 752.0f;
const sf::Color kTrainFillColor(200, 210, 223);
const sf::Color kTrainOutlineColor(70, 97, 138);

float length(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

sf::Vector2f normalizeOrFallback(sf::Vector2f vector, sf::Vector2f fallback) {
    const float vectorLength = length(vector);
    if (vectorLength <= 0.0001f) {
        return fallback;
    }

    return vector / vectorLength;
}

float clamp01(float value) {
    if (value <= 0.0f) {
        return 0.0f;
    }

    if (value >= 1.0f) {
        return 1.0f;
    }

    return value;
}

sf::Vector2f normalFromTangent(sf::Vector2f tangent) {
    return {-tangent.y, tangent.x};
}
} // namespace

MemoryReadAnimation::MemoryReadAnimation(const sf::Font* font) : m_font(font) {
    const float radius = kTrainWidth * 0.5f;
    m_headCap.setRadius(radius);
    m_headCap.setOrigin({radius, radius});
    m_headCap.setFillColor(kTrainFillColor);
    m_headCap.setOutlineThickness(3.0f);
    m_headCap.setOutlineColor(kTrainOutlineColor);

    m_tailCap.setRadius(radius);
    m_tailCap.setOrigin({radius, radius});
    m_tailCap.setFillColor(kTrainFillColor);
    m_tailCap.setOutlineThickness(3.0f);
    m_tailCap.setOutlineColor(kTrainOutlineColor);
}

void MemoryReadAnimation::setFont(const sf::Font* font) {
    m_font = font;
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
    m_hasRoute = true;
    m_visible = false;
}

void MemoryReadAnimation::sync(const sim::MemoryTransaction& transaction,
                               sim::Tick tick,
                               const view::rails::RailPath* busPath,
                               const view::rails::RailPath* installPath) {
    if (!m_hasRoute || transaction.isCompleted(tick)) {
        m_visible = false;
        return;
    }

    const float ramDistance = getToRamPortLength();
    const float busDistance = (busPath && !busPath->isEmpty()) ? busPath->getLength() : length(m_targetPosition - m_exitPosition);
    const float installDistance = (installPath && !installPath->isEmpty()) ? installPath->getLength() : 0.0f;
    const float totalDistance = ramDistance + busDistance + installDistance;

    if (totalDistance <= 0.0f) {
        rebuildCurvedBody(0.0f, totalDistance, busPath, installPath);
        m_visible = true;
        return;
    }

    const float headDistance = totalDistance * easeInOut(transaction.getOverallProgress(tick));
    const float rigidPhaseDistance = length(m_lanePosition - m_sourcePosition) +
                                     length(m_turnEntryPosition - m_lanePosition);

    if (headDistance <= rigidPhaseDistance) {
        rebuildRigidBody(sampleToRamPortByDistance(headDistance));
    } else {
        rebuildCurvedBody(headDistance, totalDistance, busPath, installPath);
    }
    m_visible = true;
}

void MemoryReadAnimation::clear() {
    m_visible = false;
}

sf::Vector2f MemoryReadAnimation::lerp(sf::Vector2f from, sf::Vector2f to, float t) {
    return from + (to - from) * t;
}

float MemoryReadAnimation::easeInOut(float t) {
    t = clamp01(t);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float MemoryReadAnimation::softEase(float t) {
    return easeInOut(t);
}

sf::Vector2f MemoryReadAnimation::sampleArcPosition(
    sf::Vector2f center, float radius, float startAngle, float endAngle, float t) {
    const float angle = startAngle + (endAngle - startAngle) * softEase(t);
    return {center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
}

sf::Vector2f MemoryReadAnimation::sampleToRamPort(float t) const {
    return sampleToRamPortByDistance(getToRamPortLength() * softEase(t));
}

float MemoryReadAnimation::getToRamPortLength() const {
    const float laneDistance = length(m_lanePosition - m_sourcePosition);
    const float toTurnEntryDistance = length(m_turnEntryPosition - m_lanePosition);
    const float firstTurnDistance = std::abs(m_turnEndAngle - m_turnStartAngle) * m_turnRadius;
    const float collectorDistance = length(m_collectorPosition - m_turnExitPosition);
    const float junctionTurnDistance =
        std::abs(m_junctionTurnEndAngle - m_junctionTurnStartAngle) * m_junctionTurnRadius;
    const float exitDistance = length(m_exitPosition - m_junctionTurnExitPosition);
    return laneDistance + toTurnEntryDistance + firstTurnDistance + collectorDistance + junctionTurnDistance +
           exitDistance;
}

sf::Vector2f MemoryReadAnimation::sampleToRamPortByDistance(float distance) const {
    const float laneDistance = length(m_lanePosition - m_sourcePosition);
    const float toTurnEntryDistance = length(m_turnEntryPosition - m_lanePosition);
    const float firstTurnDistance = std::abs(m_turnEndAngle - m_turnStartAngle) * m_turnRadius;
    const float collectorDistance = length(m_collectorPosition - m_turnExitPosition);
    const float junctionTurnDistance =
        std::abs(m_junctionTurnEndAngle - m_junctionTurnStartAngle) * m_junctionTurnRadius;
    const float exitDistance = length(m_exitPosition - m_junctionTurnExitPosition);

    float remainingDistance = std::clamp(distance, 0.0f, getToRamPortLength());

    if (remainingDistance <= laneDistance) {
        return lerp(m_sourcePosition, m_lanePosition, laneDistance > 0.0f ? remainingDistance / laneDistance : 1.0f);
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

sf::Vector2f MemoryReadAnimation::sampleToRamPortTangentByDistance(float distance) const {
    const float totalDistance = getToRamPortLength();
    const float sampleStep = 3.0f;
    const float clampedDistance = std::clamp(distance, 0.0f, totalDistance);
    const float beforeDistance = std::max(0.0f, clampedDistance - sampleStep);
    const float afterDistance = std::min(totalDistance, clampedDistance + sampleStep);
    return normalizeOrFallback(sampleToRamPortByDistance(afterDistance) - sampleToRamPortByDistance(beforeDistance),
                               {-1.0f, 0.0f});
}

sf::Vector2f MemoryReadAnimation::sampleRouteCenterByDistance(float distance,
                                                              const view::rails::RailPath* busPath,
                                                              const view::rails::RailPath* installPath) const {
    const float ramDistance = getToRamPortLength();
    const float busDistance = (busPath && !busPath->isEmpty()) ? busPath->getLength() : length(m_targetPosition - m_exitPosition);
    const float installDistance = (installPath && !installPath->isEmpty()) ? installPath->getLength() : 0.0f;
    const float totalDistance = ramDistance + busDistance + installDistance;
    const sf::Vector2f startCenter = m_sourcePosition;

    if (distance <= 0.0f) {
        return startCenter + sf::Vector2f{-1.0f, 0.0f} * distance;
    }

    float remainingDistance = std::min(distance, totalDistance);
    if (remainingDistance <= ramDistance) {
        return sampleToRamPortByDistance(remainingDistance);
    }
    remainingDistance -= ramDistance;

    if (remainingDistance <= busDistance) {
        if (busPath && !busPath->isEmpty()) {
            return busPath->samplePoint(remainingDistance);
        }

        return lerp(startCenter, m_targetPosition, busDistance > 0.0f ? remainingDistance / busDistance : 1.0f);
    }
    remainingDistance -= busDistance;

    if (installPath && !installPath->isEmpty() && installDistance > 0.0f) {
        return installPath->samplePoint(remainingDistance);
    }

    return m_targetPosition;
}

sf::Vector2f MemoryReadAnimation::sampleRouteTangentByDistance(float distance,
                                                               float totalDistance,
                                                               const view::rails::RailPath* busPath,
                                                               const view::rails::RailPath* installPath) const {
    const float ramDistance = getToRamPortLength();
    const float busDistance =
        (busPath && !busPath->isEmpty()) ? busPath->getLength() : length(m_targetPosition - m_exitPosition);
    const float installDistance = (installPath && !installPath->isEmpty()) ? installPath->getLength() : 0.0f;

    if (distance <= 0.0f) {
        return sampleToRamPortTangentByDistance(0.0f);
    }

    float remainingDistance = std::min(distance, totalDistance);
    if (remainingDistance <= ramDistance) {
        return sampleToRamPortTangentByDistance(remainingDistance);
    }
    remainingDistance -= ramDistance;

    if (remainingDistance <= busDistance) {
        if (busPath && !busPath->isEmpty()) {
            return normalizeOrFallback(busPath->sampleTangent(remainingDistance), {-1.0f, 0.0f});
        }

        return normalizeOrFallback(m_targetPosition - m_exitPosition, {-1.0f, 0.0f});
    }
    remainingDistance -= busDistance;

    if (installPath && !installPath->isEmpty() && installDistance > 0.0f) {
        return normalizeOrFallback(installPath->sampleTangent(remainingDistance), {-1.0f, 0.0f});
    }

    return normalizeOrFallback(m_targetPosition - m_exitPosition, {-1.0f, 0.0f});
}

void MemoryReadAnimation::rebuildRigidBody(sf::Vector2f headCenter) {
    m_bodyFill.clear();
    m_leftOutline.clear();
    m_rightOutline.clear();

    const sf::Vector2f direction{1.0f, 0.0f};
    const sf::Vector2f normal = normalFromTangent(direction);
    const float halfWidth = kTrainWidth * 0.5f;
    const float centerSpan = std::max(0.0f, kTrainLength - kTrainWidth);

    m_bodyFill.resize(kCurveSampleCount * 2);
    m_leftOutline.resize(kCurveSampleCount);
    m_rightOutline.resize(kCurveSampleCount);

    for (std::size_t index = 0; index < kCurveSampleCount; ++index) {
        const float t =
            kCurveSampleCount > 1 ? static_cast<float>(index) / static_cast<float>(kCurveSampleCount - 1) : 0.0f;
        const float distanceAlongTrain = centerSpan * t;
        const sf::Vector2f center = headCenter + direction * distanceAlongTrain;
        const sf::Vector2f left = center + normal * halfWidth;
        const sf::Vector2f right = center - normal * halfWidth;

        m_bodyFill[index * 2].position = left;
        m_bodyFill[index * 2].color = kTrainFillColor;
        m_bodyFill[index * 2 + 1].position = right;
        m_bodyFill[index * 2 + 1].color = kTrainFillColor;

        m_leftOutline[index].position = left;
        m_leftOutline[index].color = kTrainOutlineColor;
        m_rightOutline[index].position = right;
        m_rightOutline[index].color = kTrainOutlineColor;
    }

    m_headCap.setPosition(headCenter);
    m_tailCap.setPosition(headCenter + direction * centerSpan);
}

void MemoryReadAnimation::rebuildCurvedBody(float headDistance,
                                            float totalDistance,
                                            const view::rails::RailPath* busPath,
                                            const view::rails::RailPath* installPath) {
    m_bodyFill.clear();
    m_leftOutline.clear();
    m_rightOutline.clear();

    const float halfWidth = kTrainWidth * 0.5f;
    const float visibleCenterSpan = std::max(0.0f, kTrainLength - kTrainWidth);
    m_bodyFill.resize(kCurveSampleCount * 2);
    m_leftOutline.resize(kCurveSampleCount);
    m_rightOutline.resize(kCurveSampleCount);

    sf::Vector2f headCenter{0.0f, 0.0f};
    sf::Vector2f tailCenter{0.0f, 0.0f};
    for (std::size_t index = 0; index < kCurveSampleCount; ++index) {
        const float t =
            kCurveSampleCount > 1 ? static_cast<float>(index) / static_cast<float>(kCurveSampleCount - 1) : 0.0f;
        const float distanceAlongTrain = visibleCenterSpan * t;
        const float sampleDistance = headDistance - distanceAlongTrain;
        const sf::Vector2f center = sampleRouteCenterByDistance(sampleDistance, busPath, installPath);
        const sf::Vector2f tangent = sampleRouteTangentByDistance(sampleDistance, totalDistance, busPath, installPath);
        const sf::Vector2f normal = normalFromTangent(tangent);
        const sf::Vector2f left = center + normal * halfWidth;
        const sf::Vector2f right = center - normal * halfWidth;

        m_bodyFill[index * 2].position = left;
        m_bodyFill[index * 2].color = kTrainFillColor;
        m_bodyFill[index * 2 + 1].position = right;
        m_bodyFill[index * 2 + 1].color = kTrainFillColor;

        m_leftOutline[index].position = left;
        m_leftOutline[index].color = kTrainOutlineColor;
        m_rightOutline[index].position = right;
        m_rightOutline[index].color = kTrainOutlineColor;

        if (index == 0) {
            headCenter = center;
        }
        if (index == kCurveSampleCount - 1) {
            tailCenter = center;
        }
    }

    m_headCap.setPosition(headCenter);
    m_tailCap.setPosition(tailCenter);
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_hasRoute || !m_visible) {
        return;
    }

    target.draw(m_bodyFill, states);
    target.draw(m_tailCap, states);
    target.draw(m_headCap, states);
    target.draw(m_leftOutline, states);
    target.draw(m_rightOutline, states);
}
