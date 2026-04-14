#include <SFML/Graphics.hpp>

#include <filesystem>
#include <optional>

#include "Camera.h"
#include "CacheLine.h"

namespace
{
constexpr unsigned int kWindowWidth = 1600;
constexpr unsigned int kWindowHeight = 900;
constexpr float kInitialZoom = 0.5f;
constexpr sf::Vector2f kInitialCameraPosition(0.0f, 0.0f);

bool isPressed(sf::Keyboard::Key key)
{
    return sf::Keyboard::isKeyPressed(key);
}

bool loadFont(sf::Font& font)
{
    const std::filesystem::path localFont = "assets/fonts/arial.ttf";
    if (std::filesystem::exists(localFont) && font.openFromFile(localFont))
    {
        return true;
    }

    const std::filesystem::path windowsFont = "C:/Windows/Fonts/arial.ttf";
    return std::filesystem::exists(windowsFont) && font.openFromFile(windowsFont);
}

}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({kWindowWidth, kWindowHeight}),
        "CoreLab - CPU Visualizer Prototype",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setVerticalSyncEnabled(true);

    sf::View sceneView(kInitialCameraPosition, {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    Camera camera(sceneView, 650.0f, 0.1f);
    camera.reset(kInitialCameraPosition, kInitialZoom);

    sf::Font font;
    const bool hasFont = loadFont(font);
    CacheLine cacheLine(hasFont ? &font : nullptr);
    cacheLine.setPosition({-376.0f, -41.0f});

    sf::VertexArray grid(sf::PrimitiveType::Lines);
    constexpr int gridExtent = 5000;
    constexpr int gridStep = 250;
    for (int x = -gridExtent; x <= gridExtent; x += gridStep)
    {
        const sf::Color color(40, 48, 68);
        grid.append(sf::Vertex({static_cast<float>(x), static_cast<float>(-gridExtent)}, color));
        grid.append(sf::Vertex({static_cast<float>(x), static_cast<float>(gridExtent)}, color));
    }

    for (int y = -gridExtent; y <= gridExtent; y += gridStep)
    {
        const sf::Color color(40, 48, 68);
        grid.append(sf::Vertex({static_cast<float>(-gridExtent), static_cast<float>(y)}, color));
        grid.append(sf::Vertex({static_cast<float>(gridExtent), static_cast<float>(y)}, color));
    }

    sf::Clock frameClock;

    while (window.isOpen())
    {
        const float deltaSeconds = frameClock.restart().asSeconds();

        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                sceneView.setCenter(camera.getPosition());
                sceneView.setSize({static_cast<float>(resized->size.x) / camera.getZoom(),
                                   static_cast<float>(resized->size.y) / camera.getZoom()});
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
                    camera.reset(kInitialCameraPosition, kInitialZoom);
                }
            }
        }

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

        if (movement != sf::Vector2f(0.0f, 0.0f))
        {
            movement = movement.normalized();
            camera.move(movement * camera.getSpeed() * deltaSeconds);
        }

        camera.update(window);

        window.clear(sf::Color(8, 10, 18));
        window.setView(camera.getView());
        window.draw(grid);
        window.draw(cacheLine);

        window.setView(window.getDefaultView());

        window.display();
    }

    return 0;
}
