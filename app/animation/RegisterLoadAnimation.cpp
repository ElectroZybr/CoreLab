#include "animation/RegisterLoadAnimation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "view/CacheLineView.h"

namespace {
constexpr float kCellWidth = view::CacheLineView::kWidth / static_cast<float>(view::CacheLineView::kFloatCount);
constexpr float kCellHeight = view::CacheLineView::kHeight;
constexpr float kCornerRadius = 12.0f;
constexpr int kCornerPointCount = 12;
constexpr unsigned int kTextSize = 16;
constexpr float kHighlightDuration = 0.22f;
const sf::Color kFillColor(247, 214, 92);
const sf::Color kOutlineColor(153, 117, 18);
const sf::Color kTextColor(73, 50, 6);

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

sf::Vector2f normalFromTangent(sf::Vector2f tangent) {
    return {-tangent.y, tangent.x};
}

sf::Vector2f toWorld(sf::Vector2f origin, sf::Vector2f tangent, sf::Vector2f normal, float localX, float localY) {
    return origin + tangent * localX + normal * localY;
}

std::array<sf::Vector2f, 4 * kCornerPointCount> buildRoundedRectPoints() {
    constexpr float halfPi = std::numbers::pi_v<float> * 0.5f;
    constexpr std::array<float, 4> arcOffsets{
        std::numbers::pi_v<float>, std::numbers::pi_v<float> * 1.5f, 0.0f, halfPi};

    const std::array<sf::Vector2f, 4> centers{sf::Vector2f(kCornerRadius, kCornerRadius),
                                              sf::Vector2f(kCellWidth - kCornerRadius, kCornerRadius),
                                              sf::Vector2f(kCellWidth - kCornerRadius, kCellHeight - kCornerRadius),
                                              sf::Vector2f(kCornerRadius, kCellHeight - kCornerRadius)};

    std::array<sf::Vector2f, 4 * kCornerPointCount> points{};
    std::size_t pointIndex = 0;
    for (std::size_t cornerIndex = 0; cornerIndex < centers.size(); ++cornerIndex) {
        for (int step = 0; step < kCornerPointCount; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(kCornerPointCount - 1);
            const float angle = arcOffsets[cornerIndex] + t * halfPi;
            points[pointIndex++] = {centers[cornerIndex].x + std::cos(angle) * kCornerRadius,
                                    centers[cornerIndex].y + std::sin(angle) * kCornerRadius};
        }
    }

    return points;
}

float easeIn(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * t;
}
} // namespace

RegisterLoadAnimation::RegisterLoadAnimation(const sf::Font* fontValue) : font(fontValue) {
    body.setFillColor(kFillColor);
    body.setOutlineThickness(0.0f);
    body.setOutlineColor(kOutlineColor);
    basePoints = buildRoundedRectPoints();
    basePointCount = basePoints.size();
    body.setPointCount(basePointCount);
    for (std::size_t index = 0; index < basePointCount; ++index) {
        body.setPoint(index, basePoints[index]);
    }
}

void RegisterLoadAnimation::setFont(const sf::Font* fontValue) {
    font = fontValue;
    if (!label.empty() && font) {
        text.emplace(*font, label, kTextSize);
        text->setFillColor(kTextColor);
    }
}

void RegisterLoadAnimation::start(view::rails::RailPath pathValue,
                                  std::string labelValue,
                                  sf::Vector2f targetCenterValue) {
    path = std::move(pathValue);
    label = std::move(labelValue);
    targetCenter = targetCenterValue;
    routeLength = path.getLength();
    traveledDistance = 0.0f;
    active = !path.isEmpty();
    highlightProgress = 0.0f;
    moveProgress = 0.0f;
    phase = active ? Phase::Highlighting : Phase::Idle;

    if (font && !label.empty()) {
        text.emplace(*font, label, kTextSize);
        text->setFillColor(kTextColor);
    } else {
        text.reset();
    }

    if (active) {
        rebuildGeometry(path.getStartPoint(), normalizeOrFallback(path.sampleTangent(0.0f), {-1.0f, 0.0f}));
    }
}

void RegisterLoadAnimation::clear() {
    active = false;
    path = {};
    traveledDistance = 0.0f;
    routeLength = 0.0f;
    highlightProgress = 0.0f;
    moveProgress = 0.0f;
    phase = Phase::Idle;
}

bool RegisterLoadAnimation::update(float deltaSeconds, float pixelsPerSecond) {
    if (!active || path.isEmpty()) {
        return false;
    }

    if (phase == Phase::Highlighting) {
        highlightProgress = std::min(1.0f, highlightProgress + deltaSeconds / kHighlightDuration);
        if (highlightProgress < 1.0f) {
            return false;
        }
        phase = Phase::Moving;
    }

    const sf::Vector2f endPoint = path.getEndPoint();
    const float internalLength = std::max(0.0f, endPoint.x - targetCenter.x);
    const float totalLength = routeLength + internalLength;
    const float totalDuration = totalLength / std::max(pixelsPerSecond, 1.0f);
    if (totalDuration <= 0.0001f) {
        moveProgress = 1.0f;
    } else {
        moveProgress = std::min(1.0f, moveProgress + deltaSeconds / totalDuration);
    }
    traveledDistance = totalLength * easeIn(moveProgress);

    sf::Vector2f center = endPoint;
    sf::Vector2f tangent = normalizeOrFallback(path.sampleTangent(routeLength), {-1.0f, 0.0f});

    if (traveledDistance <= routeLength) {
        center = path.samplePoint(traveledDistance);
        tangent = normalizeOrFallback(path.sampleTangent(traveledDistance), {-1.0f, 0.0f});
    } else {
        const float insideDistance = traveledDistance - routeLength;
        center = {endPoint.x - insideDistance, targetCenter.y};
        tangent = {-1.0f, 0.0f};
    }

    rebuildGeometry(center, tangent);

    if (moveProgress >= 1.0f || traveledDistance >= totalLength - 0.5f) {
        active = false;
        phase = Phase::Idle;
        return true;
    }

    return false;
}

void RegisterLoadAnimation::rebuildGeometry(sf::Vector2f center, sf::Vector2f tangent) {
    const sf::Vector2f normal = normalFromTangent(tangent);
    const sf::Vector2f origin = center - tangent * (kCellWidth * 0.5f) - normal * (kCellHeight * 0.5f);
    for (std::size_t index = 0; index < basePointCount; ++index) {
        const sf::Vector2f local = basePoints[index];
        body.setPoint(index, toWorld(origin, tangent, normal, local.x, local.y));
    }

    if (text) {
        const sf::FloatRect bounds = text->getLocalBounds();
        text->setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
        text->setRotation(sf::degrees(std::atan2(tangent.y, tangent.x) * 180.0f / std::numbers::pi_v<float>));
        text->setPosition(center);
    }
}

void RegisterLoadAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!active) {
        return;
    }

    target.draw(body, states);
    if (text) {
        target.draw(*text, states);
    }
}
