#include "Camera.h"

#include <algorithm>

namespace {
constexpr float kMinZoom = 0.25f;
constexpr float kMaxZoom = 500.0f;
} // namespace

Camera::Camera(sf::View& view, float moveSpeed, float zoomSpeed)
    : m_view(&view), m_moveSpeed(moveSpeed), m_zoomSpeed(zoomSpeed) {
    setZoom(1.0f);
}

void Camera::update(const sf::RenderTarget& target) {
    m_screenSize = toVector2f(target.getSize());
    m_screenSize.x = std::max(m_screenSize.x, 1.0f);
    m_screenSize.y = std::max(m_screenSize.y, 1.0f);
    m_view->setCenter(m_position);
    m_view->setSize({m_screenSize.x / m_zoom, m_screenSize.y / m_zoom});
}

void Camera::setZoom(float zoom) {
    m_zoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    m_speed = m_moveSpeed / m_zoom;
}

void Camera::zoomAt(float factor, sf::Vector2i mousePixel) {
    m_zoom *= (1.0f + factor * m_zoomSpeed);
    m_zoom = std::clamp(m_zoom, kMinZoom, kMaxZoom);
    m_speed = m_moveSpeed / m_zoom;

    if (m_zoom > kMinZoom && m_zoom < kMaxZoom) {
        const sf::Vector2i screenCenter(static_cast<int>(m_screenSize.x * 0.5f),
                                        static_cast<int>(m_screenSize.y * 0.5f));
        const sf::Vector2i deltaPixel = mousePixel - screenCenter;
        m_position += sf::Vector2f(static_cast<float>(deltaPixel.x), static_cast<float>(deltaPixel.y)) *
                      (0.1f / m_zoom) * factor;
    }
}

void Camera::beginDrag(sf::Vector2i pixelPos) {
    m_isDragging = true;
    m_dragStartPixelPos = pixelPos;
    m_dragStartCameraPos = m_position;
}

void Camera::dragTo(sf::Vector2i pixelPos) {
    const sf::Vector2f deltaWorld = screenToWorld(m_dragStartPixelPos) - screenToWorld(pixelPos);
    m_position = m_dragStartCameraPos + deltaWorld;
}

sf::Vector2f Camera::screenToWorld(sf::Vector2i screenPos) const {
    if (m_screenSize.x <= 0.0f || m_screenSize.y <= 0.0f) {
        return m_position;
    }

    const sf::Vector2f viewSize = m_view->getSize();
    const sf::Vector2f viewCenter = m_view->getCenter();

    return {viewCenter.x +
                (static_cast<float>(screenPos.x) - m_screenSize.x * 0.5f) * (viewSize.x / m_screenSize.x),
            viewCenter.y +
                (static_cast<float>(screenPos.y) - m_screenSize.y * 0.5f) * (viewSize.y / m_screenSize.y)};
}

void Camera::reset(sf::Vector2f position, float zoom) {
    m_position = position;
    setZoom(zoom);
}

sf::Vector2f Camera::toVector2f(sf::Vector2u size) {
    return {static_cast<float>(size.x), static_cast<float>(size.y)};
}
