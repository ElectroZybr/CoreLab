#pragma once

#include <SFML/Graphics.hpp>

class Camera
{
public:
    explicit Camera(sf::View& view, float moveSpeed = 500.0f, float zoomSpeed = 0.1f);

    void update(const sf::RenderTarget& target);

    void move(sf::Vector2f offset) { m_position += offset; }
    void setPosition(sf::Vector2f position) { m_position = position; }
    [[nodiscard]] sf::Vector2f getPosition() const { return m_position; }

    void setZoom(float zoom);
    [[nodiscard]] float getZoom() const { return m_zoom; }
    [[nodiscard]] float getSpeed() const { return m_speed; }

    void zoomAt(float factor, sf::Vector2i mousePixel);

    void beginDrag(sf::Vector2i pixelPos);
    void dragTo(sf::Vector2i pixelPos);
    void endDrag() { m_isDragging = false; }
    [[nodiscard]] bool isDragging() const { return m_isDragging; }

    [[nodiscard]] sf::Vector2f screenToWorld(sf::Vector2i screenPos) const;

    void reset(sf::Vector2f position, float zoom);

    [[nodiscard]] sf::View& getView() { return *m_view; }
    [[nodiscard]] const sf::View& getView() const { return *m_view; }

private:
    [[nodiscard]] static sf::Vector2f toVector2f(sf::Vector2u size);

    sf::View* m_view = nullptr;
    sf::Vector2f m_screenSize{1600.0f, 900.0f};
    sf::Vector2f m_position{0.0f, 0.0f};
    float m_zoom = 1.0f;
    float m_speed = 500.0f;
    float m_moveSpeed = 500.0f;
    float m_zoomSpeed = 0.1f;

    bool m_isDragging = false;
    sf::Vector2i m_dragStartPixelPos{0, 0};
    sf::Vector2f m_dragStartCameraPos{0.0f, 0.0f};
};
