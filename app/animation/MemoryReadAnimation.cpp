#include "animation/MemoryReadAnimation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {
constexpr std::size_t kCurveSampleCount = 180;
constexpr std::size_t kFloatBlockCount = view::CacheLineView::kFloatCount;
constexpr float kAnimatedTrainWidth = 82.0f;
constexpr float kAnimatedTrainLength = 752.0f;
constexpr float kTrainCornerRadius = 16.0f;
constexpr unsigned int kBlockTextSize = 16;
constexpr std::size_t kCapCornerSampleCount = 6;
constexpr float kCapLengthScale = 0.22f;
constexpr float kCapCornerRadiusScale = 0.18f;
const sf::Color kTrainFillColor(200, 210, 223);
const sf::Color kTrainOutlineColor(70, 97, 138);
const sf::Color kTrainTextColor(27, 40, 67);
constexpr std::array<const char*, kFloatBlockCount> kBlockLabels{
    "x[0]",  "y[0]",  "z[0]",  "vx[0]", "vy[0]", "vz[0]",
    "x[1]",  "y[1]",  "z[1]",  "vx[1]", "vy[1]", "vz[1]",
    "x[2]",  "y[2]",  "z[2]",  "vx[2]"}; 

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

sf::Vector2f toWorld(sf::Vector2f origin, sf::Vector2f tangent, sf::Vector2f normal, float localX, float localY) {
    return origin + tangent * localX + normal * localY;
}

float halfWidthAtOffset(float offset) {
    const float halfWidth = kAnimatedTrainWidth * 0.5f;
    const float cornerRadius = std::min(kTrainCornerRadius, halfWidth);
    const float clampedOffset = std::clamp(offset, 0.0f, kAnimatedTrainLength);

    auto edgeHalfWidth = [halfWidth, cornerRadius](float distanceFromFace) {
        if (distanceFromFace >= cornerRadius) {
            return halfWidth;
        }

        const float x = cornerRadius - distanceFromFace;
        return (halfWidth - cornerRadius) +
               std::sqrt(std::max(0.0f, cornerRadius * cornerRadius - x * x));
    };

    return std::min(edgeHalfWidth(clampedOffset), edgeHalfWidth(kAnimatedTrainLength - clampedOffset));
}

void appendArcPoints(std::vector<sf::Vector2f>& points,
                     sf::Vector2f origin,
                     sf::Vector2f tangent,
                     sf::Vector2f normal,
                     float centerX,
                     float centerY,
                     float radius,
                     float startAngle,
                     float endAngle) {
    for (std::size_t index = 0; index <= kCapCornerSampleCount; ++index) {
        const float t = static_cast<float>(index) / static_cast<float>(kCapCornerSampleCount);
        const float angle = startAngle + (endAngle - startAngle) * t;
        const float localX = centerX + std::cos(angle) * radius;
        const float localY = centerY + std::sin(angle) * radius;
        points.push_back(toWorld(origin, tangent, normal, localX, localY));
    }
}

void buildEndCap(sf::VertexArray& fill,
                 sf::VertexArray& outline,
                 sf::Vector2f baseCenter,
                 sf::Vector2f tangent,
                 sf::Vector2f normal,
                 bool isHead) {
    const float halfWidth = kAnimatedTrainWidth * 0.5f;
    const float capLength = kAnimatedTrainWidth * kCapLengthScale;
    const float cornerRadius = std::min(kAnimatedTrainWidth * kCapCornerRadiusScale, capLength);
    const float signedTangent = isHead ? 1.0f : -1.0f;
    const float outerX = capLength * signedTangent;
    const float cornerCenterX = (capLength - cornerRadius) * signedTangent;

    std::vector<sf::Vector2f> polygon;
    polygon.reserve(2 * (kCapCornerSampleCount + 1) + 4);
    polygon.push_back(toWorld(baseCenter, tangent, normal, 0.0f, halfWidth));
    polygon.push_back(toWorld(baseCenter, tangent, normal, cornerCenterX, halfWidth));

    if (isHead) {
        appendArcPoints(polygon,
                        baseCenter,
                        tangent,
                        normal,
                        cornerCenterX,
                        halfWidth - cornerRadius,
                        cornerRadius,
                        std::numbers::pi_v<float> * 0.5f,
                        0.0f);
        appendArcPoints(polygon,
                        baseCenter,
                        tangent,
                        normal,
                        cornerCenterX,
                        -halfWidth + cornerRadius,
                        cornerRadius,
                        0.0f,
                        -std::numbers::pi_v<float> * 0.5f);
    } else {
        appendArcPoints(polygon,
                        baseCenter,
                        tangent,
                        normal,
                        cornerCenterX,
                        halfWidth - cornerRadius,
                        cornerRadius,
                        std::numbers::pi_v<float> * 0.5f,
                        std::numbers::pi_v<float>);
        appendArcPoints(polygon,
                        baseCenter,
                        tangent,
                        normal,
                        cornerCenterX,
                        -halfWidth + cornerRadius,
                        cornerRadius,
                        std::numbers::pi_v<float>,
                        std::numbers::pi_v<float> * 1.5f);
    }

    polygon.push_back(toWorld(baseCenter, tangent, normal, 0.0f, -halfWidth));

    fill.clear();
    outline.clear();
    fill.resize(polygon.size() + 2);
    outline.resize(polygon.size() + 1);

    sf::Vector2f centroid{0.0f, 0.0f};
    for (const sf::Vector2f& point : polygon) {
        centroid += point;
    }
    centroid /= static_cast<float>(polygon.size());

    fill[0].position = centroid;
    fill[0].color = kTrainFillColor;
    for (std::size_t index = 0; index < polygon.size(); ++index) {
        fill[index + 1].position = polygon[index];
        fill[index + 1].color = kTrainFillColor;
        outline[index].position = polygon[index];
        outline[index].color = kTrainOutlineColor;
    }
    fill[polygon.size() + 1].position = polygon.front();
    fill[polygon.size() + 1].color = kTrainFillColor;
    outline[polygon.size()].position = polygon.front();
    outline[polygon.size()].color = kTrainOutlineColor;
}
} // namespace

MemoryReadAnimation::MemoryReadAnimation(const sf::Font* font) : m_font(font) {
    rebuildText();
}

void MemoryReadAnimation::setFont(const sf::Font* font) {
    m_font = font;
    rebuildText();
}

void MemoryReadAnimation::setCellLabels(const sim::RAM::LineCellLabels& labels) {
    if (!m_font) {
        return;
    }

    for (std::size_t index = 0; index < m_blockTexts.size(); ++index) {
        m_blockTexts[index].reset();
        if (labels[index].empty()) {
            continue;
        }

        m_blockTexts[index].emplace(*m_font, labels[index], kBlockTextSize);
        m_blockTexts[index]->setFillColor(kTrainTextColor);
    }
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
    if (!m_hasRoute) {
        m_visible = false;
        return;
    }

    const float ramDistance = getToRamPortLength();
    const float busDistance = (busPath && !busPath->isEmpty()) ? busPath->getLength() : length(m_targetPosition - m_exitPosition);
    const float installDistance = (installPath && !installPath->isEmpty()) ? installPath->getLength() : 0.0f;
    const float totalDistance = ramDistance + busDistance + installDistance;

    if (transaction.isCompleted(tick)) {
        rebuildCurvedBody(totalDistance, totalDistance, busPath, installPath);
        m_visible = true;
        return;
    }

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

void MemoryReadAnimation::rebuildText() {
    for (std::optional<sf::Text>& blockText : m_blockTexts) {
        blockText.reset();
    }

    if (!m_font) {
        return;
    }

    for (std::size_t index = 0; index < m_blockTexts.size(); ++index) {
        m_blockTexts[index].emplace(*m_font, kBlockLabels[index], kBlockTextSize);
        m_blockTexts[index]->setFillColor(kTrainTextColor);
    }
}

void MemoryReadAnimation::rebuildRigidBody(sf::Vector2f headCenter) {
    m_bodyFill.clear();
    m_leftOutline.clear();
    m_rightOutline.clear();
    m_dividers.clear();
    m_headCap.clear();
    m_tailCap.clear();
    m_headOutline.clear();
    m_tailOutline.clear();

    const sf::Vector2f trailingDirection{1.0f, 0.0f};
    const sf::Vector2f forwardTangent{-1.0f, 0.0f};
    const sf::Vector2f normal = normalFromTangent(forwardTangent);
    const float headFaceHalfWidth = halfWidthAtOffset(0.0f);
    const float tailFaceHalfWidth = halfWidthAtOffset(kTrainLength);

    m_bodyFill.resize(kCurveSampleCount * 2);
    m_leftOutline.resize(kCurveSampleCount);
    m_rightOutline.resize(kCurveSampleCount);

    for (std::size_t index = 0; index < kCurveSampleCount; ++index) {
        const float t =
            kCurveSampleCount > 1 ? static_cast<float>(index) / static_cast<float>(kCurveSampleCount - 1) : 0.0f;
        const float distanceAlongTrain = kTrainLength * t;
        const float halfWidth = halfWidthAtOffset(distanceAlongTrain);
        const sf::Vector2f center = headCenter + trailingDirection * distanceAlongTrain;
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

    const float blockStride = kTrainLength / static_cast<float>(kFloatBlockCount);
    m_dividers.resize((kFloatBlockCount - 1) * 2);
    for (std::size_t index = 1; index < kFloatBlockCount; ++index) {
        const float offset = blockStride * static_cast<float>(index);
        const float halfWidth = halfWidthAtOffset(offset);
        const sf::Vector2f dividerCenter = headCenter + trailingDirection * offset;
        const sf::Vector2f top = dividerCenter + normal * halfWidth;
        const sf::Vector2f bottom = dividerCenter - normal * halfWidth;
        m_dividers[(index - 1) * 2].position = top;
        m_dividers[(index - 1) * 2].color = kTrainOutlineColor;
        m_dividers[(index - 1) * 2 + 1].position = bottom;
        m_dividers[(index - 1) * 2 + 1].color = kTrainOutlineColor;
    }

    for (std::size_t index = 0; index < m_blockTexts.size(); ++index) {
        if (!m_blockTexts[index]) {
            continue;
        }

        const float offset = blockStride * (static_cast<float>(index) + 0.5f);
        const sf::Vector2f labelCenter = headCenter + trailingDirection * offset;
        sf::Text& text = *m_blockTexts[index];
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
        text.setRotation(sf::degrees(0.0f));
        text.setPosition(labelCenter);
    }

    m_headOutline.resize(2);
    m_headOutline[0].position = headCenter + normal * headFaceHalfWidth;
    m_headOutline[0].color = kTrainOutlineColor;
    m_headOutline[1].position = headCenter - normal * headFaceHalfWidth;
    m_headOutline[1].color = kTrainOutlineColor;

    const sf::Vector2f tailCenter = headCenter + trailingDirection * kTrainLength;
    m_tailOutline.resize(2);
    m_tailOutline[0].position = tailCenter + normal * tailFaceHalfWidth;
    m_tailOutline[0].color = kTrainOutlineColor;
    m_tailOutline[1].position = tailCenter - normal * tailFaceHalfWidth;
    m_tailOutline[1].color = kTrainOutlineColor;
}

void MemoryReadAnimation::rebuildCurvedBody(float headDistance,
                                            float totalDistance,
                                            const view::rails::RailPath* busPath,
                                            const view::rails::RailPath* installPath) {
    m_bodyFill.clear();
    m_leftOutline.clear();
    m_rightOutline.clear();
    m_dividers.clear();
    m_headCap.clear();
    m_tailCap.clear();
    m_headOutline.clear();
    m_tailOutline.clear();

    m_bodyFill.resize(kCurveSampleCount * 2);
    m_leftOutline.resize(kCurveSampleCount);
    m_rightOutline.resize(kCurveSampleCount);

    sf::Vector2f headCenter{0.0f, 0.0f};
    sf::Vector2f tailCenter{0.0f, 0.0f};
    for (std::size_t index = 0; index < kCurveSampleCount; ++index) {
        const float t =
            kCurveSampleCount > 1 ? static_cast<float>(index) / static_cast<float>(kCurveSampleCount - 1) : 0.0f;
        const float distanceAlongTrain = kTrainLength * t;
        const float sampleDistance = headDistance - distanceAlongTrain;
        const sf::Vector2f center = sampleRouteCenterByDistance(sampleDistance, busPath, installPath);
        const sf::Vector2f tangent = sampleRouteTangentByDistance(sampleDistance, totalDistance, busPath, installPath);
        const sf::Vector2f normal = normalFromTangent(tangent);
        const float halfWidth = halfWidthAtOffset(distanceAlongTrain);
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

    const float blockStride = kTrainLength / static_cast<float>(kFloatBlockCount);
    m_dividers.resize((kFloatBlockCount - 1) * 2);
    for (std::size_t index = 1; index < kFloatBlockCount; ++index) {
        const float offset = blockStride * static_cast<float>(index);
        const float sampleDistance = headDistance - offset;
        const sf::Vector2f dividerCenter = sampleRouteCenterByDistance(sampleDistance, busPath, installPath);
        const sf::Vector2f dividerTangent =
            sampleRouteTangentByDistance(sampleDistance, totalDistance, busPath, installPath);
        const sf::Vector2f dividerNormal = normalFromTangent(dividerTangent);
        const float halfWidth = halfWidthAtOffset(offset);
        const sf::Vector2f top = dividerCenter + dividerNormal * halfWidth;
        const sf::Vector2f bottom = dividerCenter - dividerNormal * halfWidth;
        m_dividers[(index - 1) * 2].position = top;
        m_dividers[(index - 1) * 2].color = kTrainOutlineColor;
        m_dividers[(index - 1) * 2 + 1].position = bottom;
        m_dividers[(index - 1) * 2 + 1].color = kTrainOutlineColor;
    }

    for (std::size_t index = 0; index < m_blockTexts.size(); ++index) {
        if (!m_blockTexts[index]) {
            continue;
        }

        const float offset = blockStride * (static_cast<float>(index) + 0.5f);
        const float sampleDistance = headDistance - offset;
        sf::Vector2f bodyAxis =
            -sampleRouteTangentByDistance(sampleDistance, totalDistance, busPath, installPath);
        bodyAxis = normalizeOrFallback(bodyAxis, {1.0f, 0.0f});

        sf::Text& text = *m_blockTexts[index];
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
        text.setRotation(sf::degrees(std::atan2(bodyAxis.y, bodyAxis.x) * 180.0f / std::numbers::pi_v<float>));
        text.setPosition(sampleRouteCenterByDistance(sampleDistance, busPath, installPath));
    }

    const sf::Vector2f headTangent =
        sampleRouteTangentByDistance(headDistance, totalDistance, busPath, installPath);
    const sf::Vector2f tailTangent =
        sampleRouteTangentByDistance(headDistance - kTrainLength, totalDistance, busPath, installPath);
    const sf::Vector2f headNormal = normalFromTangent(headTangent);
    const sf::Vector2f tailNormal = normalFromTangent(tailTangent);

    const float headFaceHalfWidth = halfWidthAtOffset(0.0f);
    const float tailFaceHalfWidth = halfWidthAtOffset(kTrainLength);

    m_headOutline.resize(2);
    m_headOutline[0].position = headCenter + headNormal * headFaceHalfWidth;
    m_headOutline[0].color = kTrainOutlineColor;
    m_headOutline[1].position = headCenter - headNormal * headFaceHalfWidth;
    m_headOutline[1].color = kTrainOutlineColor;

    m_tailOutline.resize(2);
    m_tailOutline[0].position = tailCenter + tailNormal * tailFaceHalfWidth;
    m_tailOutline[0].color = kTrainOutlineColor;
    m_tailOutline[1].position = tailCenter - tailNormal * tailFaceHalfWidth;
    m_tailOutline[1].color = kTrainOutlineColor;
}

void MemoryReadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_hasRoute || !m_visible) {
        return;
    }

    target.draw(m_bodyFill, states);
    target.draw(m_dividers, states);

    for (const std::optional<sf::Text>& blockText : m_blockTexts) {
        if (blockText) {
            target.draw(*blockText, states);
        }
    }

    target.draw(m_leftOutline, states);
    target.draw(m_rightOutline, states);
    target.draw(m_headOutline, states);
    target.draw(m_tailOutline, states);
}
