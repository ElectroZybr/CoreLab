#include <SFML/Graphics.hpp>

#include <array>

#include "animation/MemoryReadAnimation.h"
#include "core/Camera.h"
#include "input/Controls.h"
#include "objects/RAM.h"
#include "scene/Scene.h"

namespace
{
constexpr unsigned int kWindowWidth = 1600;
constexpr unsigned int kWindowHeight = 900;
constexpr float kInitialZoom = 0.5f;
constexpr float kRamMoveSpeed = 900.0f;
constexpr sf::Vector2f kInitialCameraPosition(0.0f, 0.0f);
const sf::Color kBackgroundColor(8, 10, 18);
constexpr std::array<std::size_t, 4> kAnimatedBlocks{8, 9, 10, 11};
}

int main()
{
    sf::ContextSettings contextSettings;
    contextSettings.antiAliasingLevel = 8;

    sf::RenderWindow window(
        sf::VideoMode({kWindowWidth, kWindowHeight}),
        "CoreLab - CPU Visualizer Prototype",
        sf::Style::Titlebar | sf::Style::Close,
        sf::State::Windowed,
        contextSettings
    );
    window.setVerticalSyncEnabled(true);

    sf::View sceneView(kInitialCameraPosition, {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    Camera camera(sceneView, 650.0f, 0.1f);
    camera.reset(kInitialCameraPosition, kInitialZoom);

    Scene scene;
    RAM ram(4096, scene.getFont());
    ram.setPosition({-2392.0f, 60.0f});
    MemoryReadAnimation readAnimation(scene.getFont());
    std::size_t animatedBlockIndex = 0;

    const auto applyAnimatedBlockRoute = [&]()
    {
        const RAM::ReadPath readPath = ram.getReadPath(kAnimatedBlocks[animatedBlockIndex]);
        readAnimation.setRoute(
            readPath.sourcePosition,
            readPath.lanePosition,
            readPath.turnEntryPosition,
            readPath.turnCenter,
            readPath.turnRadius,
            readPath.turnExitPosition,
            readPath.exitPosition,
            scene.getCacheLineEntryPosition()
        );
    };

    applyAnimatedBlockRoute();

    sf::Clock frameClock;

    while (window.isOpen())
    {
        const float deltaSeconds = frameClock.restart().asSeconds();
        Controls::handleEvents(window, camera, sceneView, kInitialCameraPosition, kInitialZoom);

        sf::Vector2f movement = Controls::readMovement();

        if (movement != sf::Vector2f(0.0f, 0.0f))
        {
            movement = movement.normalized();
            camera.move(movement * camera.getSpeed() * deltaSeconds);
        }

        sf::Vector2f ramMovement = Controls::readRamMovement();
        if (ramMovement != sf::Vector2f(0.0f, 0.0f))
        {
            ramMovement = ramMovement.normalized();
            ram.setPosition(ram.getPosition() + ramMovement * kRamMoveSpeed * deltaSeconds);
            applyAnimatedBlockRoute();
        }

        readAnimation.update(deltaSeconds);
        if (readAnimation.consumeCycleCompleted())
        {
            animatedBlockIndex = (animatedBlockIndex + 1) % kAnimatedBlocks.size();
            applyAnimatedBlockRoute();
        }

        camera.update(window);

        window.clear(kBackgroundColor);
        window.setView(camera.getView());
        window.draw(scene);
        window.draw(ram);
        window.draw(readAnimation);
        window.display();
    }

    return 0;
}
