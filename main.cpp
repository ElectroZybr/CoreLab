#include <SFML/Graphics.hpp>

#include <array>
#include <cmath>

#include "animation/MemoryReadAnimation.h"
#include "core/Camera.h"
#include "input/Controls.h"
#include "scene/Scene.h"
#include "sim/simulation.h"
#include "view/BusView.h"
#include "view/CacheView.h"
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

sf::Vector2f normalizeOrZero(sf::Vector2f vector) {
    const float lengthSquared = vector.x * vector.x + vector.y * vector.y;
    if (lengthSquared <= 0.0f) {
        return {0.0f, 0.0f};
    }

    const float invLength = 1.0f / std::sqrt(lengthSquared);
    return {vector.x * invLength, vector.y * invLength};
}

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
    view::RamView ram(simulation.getRam().getSizeInBytes(), scene.getFont(), {-2392.0f, 60.0f});
    view::CacheView cache(scene.getFont(), {-1000.0f, -1000.0f});
    view::BusView ramToCacheBus(10.0f);
    cache.sync(simulation.getCache());
    MemoryReadAnimation readAnimation(scene.getFont());
    std::size_t nextAnimatedBlockIndex = 0;
    float simulationTickAccumulator = 0.0f;

    sf::Clock frameClock;

    while (window.isOpen()) {
        const float deltaSeconds = frameClock.restart().asSeconds();
        Controls::handleEvents(window, camera, sceneView, kInitialCameraPosition, kInitialZoom);

        sf::Vector2f movement = Controls::readMovement();

        if (movement != sf::Vector2f(0.0f, 0.0f)) {
            movement = normalizeOrZero(movement);
            camera.move(movement * camera.getSpeed() * deltaSeconds);
        }

        sf::Vector2f ramMovement = Controls::readRamMovement();
        if (ramMovement != sf::Vector2f(0.0f, 0.0f)) {
            ramMovement = normalizeOrZero(ramMovement);
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
            cache.sync(simulation.getCache(), activeTransaction);
            ramToCacheBus.setEndpoints(readPath.exitPosition, cache.getEntryPosition());
            readAnimation.setRoute(readPath.sourcePosition,
                                   readPath.lanePosition,
                                   readPath.turnEntryPosition,
                                   readPath.turnCenter,
                                   readPath.turnRadius,
                                   readPath.turnExitPosition,
                                   readPath.exitCurveControl1,
                                   readPath.exitCurveControl2,
                                   readPath.exitPosition,
                                   cache.getEntryPosition());
            readAnimation.sync(*activeTransaction, simulation.getCurrentTick());
        } else {
            cache.sync(simulation.getCache());
            ramToCacheBus.clear();
            readAnimation.clear();
        }

        camera.update(window);

        window.clear(kBackgroundColor);
        window.setView(camera.getView());
        window.draw(scene);
        window.draw(ram);
        window.draw(ramToCacheBus);
        window.draw(cache);
        window.draw(readAnimation);
        window.display();
    }

    return 0;
}
