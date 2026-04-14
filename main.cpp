#include <SFML/Graphics.hpp>

#include <array>

#include "animation/MemoryReadAnimation.h"
#include "core/Camera.h"
#include "input/Controls.h"
#include "scene/Scene.h"
#include "sim/simulation.h"
#include "view/RamView.h"

namespace {
constexpr unsigned int kWindowWidth = 1600;
constexpr unsigned int kWindowHeight = 900;
constexpr float kInitialZoom = 0.5f;
constexpr float kRamMoveSpeed = 900.0f;
constexpr float kSimulationTicksPerSecond = 30.0f;
constexpr sf::Vector2f kInitialCameraPosition(0.0f, 0.0f);
const sf::Color kBackgroundColor(8, 10, 18);
constexpr std::array<std::size_t, 4> kAnimatedBlocks{8, 16, 24, 32};

sim::Address toBlockAddress(std::size_t blockIndex) {
    return static_cast<sim::Address>(blockIndex) * sim::RAM::kCacheLineSizeInBytes;
}

const sim::MemoryTransaction* findActiveTransaction(const sim::Simulation& simulation) {
    for (auto it = simulation.getTransactions().rbegin(); it != simulation.getTransactions().rend(); ++it) {
        if (!it->isCompleted(simulation.getCurrentTick())) {
            return &(*it);
        }
    }

    return nullptr;
}
} // namespace

int main() {
    sf::ContextSettings contextSettings;
    contextSettings.antiAliasingLevel = 8;

    sf::RenderWindow window(sf::VideoMode({kWindowWidth, kWindowHeight}),
                            "CoreLab - CPU Visualizer Prototype",
                            sf::Style::Titlebar | sf::Style::Close,
                            sf::State::Windowed,
                            contextSettings);
    window.setVerticalSyncEnabled(true);

    sf::View sceneView(kInitialCameraPosition,
                       {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    Camera camera(sceneView, 650.0f, 0.1f);
    camera.reset(kInitialCameraPosition, kInitialZoom);

    sim::Simulation simulation(4096);
    Scene scene;
    view::RamView ram(simulation.getRam().getSizeInBytes(), scene.getFont());
    ram.setPosition({-2392.0f, 60.0f});
    MemoryReadAnimation readAnimation(scene.getFont());
    std::size_t nextAnimatedBlockIndex = 0;
    float simulationTickAccumulator = 0.0f;

    sf::Clock frameClock;

    while (window.isOpen()) {
        const float deltaSeconds = frameClock.restart().asSeconds();
        Controls::handleEvents(window, camera, sceneView, kInitialCameraPosition, kInitialZoom);

        sf::Vector2f movement = Controls::readMovement();

        if (movement != sf::Vector2f(0.0f, 0.0f)) {
            movement = movement.normalized();
            camera.move(movement * camera.getSpeed() * deltaSeconds);
        }

        sf::Vector2f ramMovement = Controls::readRamMovement();
        if (ramMovement != sf::Vector2f(0.0f, 0.0f)) {
            ramMovement = ramMovement.normalized();
            ram.setPosition(ram.getPosition() + ramMovement * kRamMoveSpeed * deltaSeconds);
        }

        simulationTickAccumulator += deltaSeconds * kSimulationTicksPerSecond;
        while (simulationTickAccumulator >= 1.0f) {
            simulation.advance(1);
            simulationTickAccumulator -= 1.0f;
        }

        const sim::MemoryTransaction* activeTransaction = findActiveTransaction(simulation);
        if (!activeTransaction) {
            const sim::Address address = toBlockAddress(kAnimatedBlocks[nextAnimatedBlockIndex]);
            simulation.loadFloat(address);
            nextAnimatedBlockIndex = (nextAnimatedBlockIndex + 1) % kAnimatedBlocks.size();
            activeTransaction = findActiveTransaction(simulation);
        }

        if (activeTransaction) {
            const std::size_t lineIndex = static_cast<std::size_t>(activeTransaction->getLineBaseAddress() /
                                                                   sim::RAM::kCacheLineSizeInBytes);
            const view::RamView::ReadPath readPath = ram.getReadPath(lineIndex);
            readAnimation.setRoute(readPath.sourcePosition,
                                   readPath.lanePosition,
                                   readPath.turnEntryPosition,
                                   readPath.turnCenter,
                                   readPath.turnRadius,
                                   readPath.turnExitPosition,
                                   readPath.exitPosition,
                                   scene.getCacheLineEntryPosition());
            readAnimation.sync(*activeTransaction, simulation.getCurrentTick());
        } else {
            readAnimation.clear();
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
