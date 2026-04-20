#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string>

#include "view/rails/RailPath.h"

class RegisterLoadAnimation : public sf::Drawable {
  public:
    enum class Phase {
        Idle,
        Highlighting,
        Moving,
    };

    explicit RegisterLoadAnimation(const sf::Font* font = nullptr);

    void setFont(const sf::Font* font);
    void start(view::rails::RailPath path, std::string label, sf::Vector2f targetCenter);
    void clear();
    bool update(float deltaSeconds, float pixelsPerSecond);
    [[nodiscard]] bool isActive() const {
        return active;
    }
    [[nodiscard]] float getHighlightProgress() const {
        return highlightProgress;
    }
    [[nodiscard]] Phase getPhase() const {
        return phase;
    }

  private:
    void rebuildGeometry(sf::Vector2f center, sf::Vector2f tangent);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* font = nullptr;
    view::rails::RailPath path;
    std::string label;
    std::optional<sf::Text> text;
    sf::ConvexShape body;
    std::array<sf::Vector2f, 4 * 12> basePoints{};
    std::size_t basePointCount = 0;
    sf::Vector2f targetCenter{0.0f, 0.0f};
    float routeLength = 0.0f;
    float traveledDistance = 0.0f;
    float highlightProgress = 0.0f;
    float moveProgress = 0.0f;
    Phase phase = Phase::Idle;
    bool active = false;
};
