#include "Controls.h"

#include <optional>

namespace
{
bool isPressed(sf::Keyboard::Key key)
{
    return sf::Keyboard::isKeyPressed(key);
}
}

void Controls::handleEvents(
    sf::RenderWindow& window,
    Camera& camera,
    sf::View& sceneView,
    sf::Vector2f resetPosition,
    float resetZoom
)
{
    while (const std::optional<sf::Event> event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            sceneView.setCenter(camera.getPosition());
            sceneView.setSize({
                static_cast<float>(resized->size.x) / camera.getZoom(),
                static_cast<float>(resized->size.y) / camera.getZoom()
            });
        }

        if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (wheel->wheel == sf::Mouse::Wheel::Vertical)
            {
                camera.zoomAt(wheel->delta, wheel->position);
            }
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Right)
            {
                camera.beginDrag(mousePressed->position);
            }
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (mouseReleased->button == sf::Mouse::Button::Right)
            {
                camera.endDrag();
            }
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (camera.isDragging())
            {
                camera.dragTo(mouseMoved->position);
            }
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::R)
            {
                camera.reset(resetPosition, resetZoom);
            }
        }
    }
}

sf::Vector2f Controls::readMovement()
{
    sf::Vector2f movement(0.0f, 0.0f);

    if (isPressed(sf::Keyboard::Key::A) || isPressed(sf::Keyboard::Key::Left))
    {
        movement.x -= 1.0f;
    }
    if (isPressed(sf::Keyboard::Key::D) || isPressed(sf::Keyboard::Key::Right))
    {
        movement.x += 1.0f;
    }
    if (isPressed(sf::Keyboard::Key::W) || isPressed(sf::Keyboard::Key::Up))
    {
        movement.y -= 1.0f;
    }
    if (isPressed(sf::Keyboard::Key::S) || isPressed(sf::Keyboard::Key::Down))
    {
        movement.y += 1.0f;
    }

    return movement;
}
