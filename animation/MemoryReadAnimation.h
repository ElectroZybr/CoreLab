#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

#include "sim/MemoryTransaction.h"
#include "view/rails/RailPath.h"

class MemoryReadAnimation : public sf::Drawable {
  public:
    explicit MemoryReadAnimation(const sf::Font* font = nullptr);

    void setFont(const sf::Font* font);
    void setRoute(sf::Vector2f sourcePosition,
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
                  sf::Vector2f targetPosition);
    void sync(const sim::MemoryTransaction& transaction,
              sim::Tick tick,
              const view::rails::RailPath* busPath = nullptr,
              const view::rails::RailPath* installPath = nullptr);
    void clear();

  private:
    static constexpr float kTrainWidth = 82.0f;
    static constexpr float kTrainLength = 752.0f;

    [[nodiscard]] static sf::Vector2f lerp(sf::Vector2f from, sf::Vector2f to, float t);
    [[nodiscard]] static float easeInOut(float t);
    [[nodiscard]] static float softEase(float t);
    [[nodiscard]] static sf::Vector2f
    sampleArcPosition(sf::Vector2f center, float radius, float startAngle, float endAngle, float t);
    [[nodiscard]] float getToRamPortLength() const;
    [[nodiscard]] sf::Vector2f sampleToRamPortByDistance(float distance) const;
    [[nodiscard]] sf::Vector2f sampleToRamPortTangentByDistance(float distance) const;
    [[nodiscard]] sf::Vector2f sampleToRamPort(float t) const;
    [[nodiscard]] sf::Vector2f sampleRouteCenterByDistance(float distance,
                                                           const view::rails::RailPath* busPath,
                                                           const view::rails::RailPath* installPath) const;
    [[nodiscard]] sf::Vector2f sampleRouteTangentByDistance(float distance,
                                                            float totalDistance,
                                                            const view::rails::RailPath* busPath,
                                                            const view::rails::RailPath* installPath) const;
    void rebuildText();
    void rebuildRigidBody(sf::Vector2f headCenter);
    void rebuildCurvedBody(float headDistance,
                           float totalDistance,
                           const view::rails::RailPath* busPath,
                           const view::rails::RailPath* installPath);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_sourcePosition{0.0f, 0.0f};
    sf::Vector2f m_lanePosition{0.0f, 0.0f};
    sf::Vector2f m_turnEntryPosition{0.0f, 0.0f};
    sf::Vector2f m_turnCenter{0.0f, 0.0f};
    float m_turnRadius = 0.0f;
    float m_turnStartAngle = 0.0f;
    float m_turnEndAngle = 0.0f;
    sf::Vector2f m_turnExitPosition{0.0f, 0.0f};
    sf::Vector2f m_collectorPosition{0.0f, 0.0f};
    sf::Vector2f m_junctionTurnCenter{0.0f, 0.0f};
    float m_junctionTurnRadius = 0.0f;
    float m_junctionTurnStartAngle = 0.0f;
    float m_junctionTurnEndAngle = 0.0f;
    sf::Vector2f m_junctionTurnExitPosition{0.0f, 0.0f};
    sf::Vector2f m_exitPosition{0.0f, 0.0f};
    sf::Vector2f m_targetPosition{0.0f, 0.0f};
    bool m_hasRoute = false;
    bool m_visible = false;
    sf::VertexArray m_bodyFill{sf::PrimitiveType::TriangleStrip};
    sf::VertexArray m_leftOutline{sf::PrimitiveType::LineStrip};
    sf::VertexArray m_rightOutline{sf::PrimitiveType::LineStrip};
    sf::VertexArray m_dividers{sf::PrimitiveType::Lines};
    sf::VertexArray m_headCap{sf::PrimitiveType::TriangleFan};
    sf::VertexArray m_tailCap{sf::PrimitiveType::TriangleFan};
    sf::VertexArray m_headOutline{sf::PrimitiveType::LineStrip};
    sf::VertexArray m_tailOutline{sf::PrimitiveType::LineStrip};
    std::array<std::optional<sf::Text>, 8> m_blockTexts;
};
