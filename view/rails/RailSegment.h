#pragma once

#include <SFML/Graphics.hpp>

#include <span>

namespace view::rails {
class RailSegment : public sf::Drawable {
  public:
    explicit RailSegment(float thickness = 6.0f, sf::Color color = sf::Color(116, 134, 165));
    ~RailSegment() override = default;

    [[nodiscard]] float getThickness() const {
        return m_thickness;
    }
    void setThickness(float thickness);

    [[nodiscard]] sf::Color getColor() const {
        return m_color;
    }
    void setColor(sf::Color color);

    [[nodiscard]] virtual float getLength() const = 0;
    [[nodiscard]] virtual sf::Vector2f getStartPoint() const = 0;
    [[nodiscard]] virtual sf::Vector2f getEndPoint() const = 0;
    [[nodiscard]] virtual sf::Vector2f samplePoint(float distance) const = 0;
    [[nodiscard]] virtual sf::Vector2f sampleTangent(float distance) const = 0;

  protected:
    void markDirty() const {
        m_dirty = true;
    }
    [[nodiscard]] float clampDistance(float distance) const;
    [[nodiscard]] static sf::Vector2f normalizeOrZero(sf::Vector2f vector);
    [[nodiscard]] static sf::VertexArray
    buildThickPolyline(std::span<const sf::Vector2f> points, float thickness, sf::Color color);

    mutable bool m_dirty = true;
    mutable sf::VertexArray m_geometry;
    float m_thickness = 6.0f;
    sf::Color m_color;

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    virtual void rebuildGeometry() const = 0;
};
} // namespace view::rails
