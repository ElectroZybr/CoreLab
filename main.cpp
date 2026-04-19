#include <SFML/Graphics.hpp>

#include <cmath>
#include <optional>
#include <random>
#include <string>

#include "app/animation/MemoryReadAnimation.h"
#include "app/core/Camera.h"
#include "app/input/Controls.h"
#include "app/scene/Scene.h"
#include "app/sim/simulation.h"
#include "app/view/BusView.h"
#include "app/view/CacheView.h"
#include "app/view/CpuView.h"
#include "app/view/RamView.h"

namespace {
constexpr unsigned int kWindowWidth = 1600;
constexpr unsigned int kWindowHeight = 900;
constexpr float kInitialZoom = 0.5f;
constexpr float kSimulationTicksPerSecond = 60.0f;
constexpr float kRailThickness = 6.0f;
constexpr sf::Vector2f kInitialCameraPosition(0.0f, 0.0f);
const sf::Color kBackgroundColor(8, 10, 18);
constexpr float kPixelsPerTick = 55.0f;
constexpr sim::Tick kMinToRamPortTicks = 10;
constexpr sim::Tick kMinBusTicks = 18;
constexpr sim::Tick kMinInstallTicks = 12;

enum class DragTarget {
    None,
    Ram,
    Cpu,
};

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

sim::Address chooseRandomAnimatedAddress(const sim::Simulation& simulation, std::mt19937& rng) {
    const std::size_t cacheSlotCount = simulation.getCache().getSlotCount();
    const std::size_t ramLineCount = simulation.getRam().getLineCount();

    if (cacheSlotCount == 0 || ramLineCount == 0) {
        return 0;
    }

    std::uniform_int_distribution<std::size_t> slotDistribution(0, cacheSlotCount - 1);
    const std::size_t targetSlot = slotDistribution(rng);

    std::vector<std::size_t> candidateLines;
    candidateLines.reserve((ramLineCount / cacheSlotCount) + 1);
    for (std::size_t lineIndex = targetSlot; lineIndex < ramLineCount; lineIndex += cacheSlotCount) {
        candidateLines.push_back(lineIndex);
    }

    if (candidateLines.empty()) {
        return 0;
    }

    std::uniform_int_distribution<std::size_t> lineDistribution(0, candidateLines.size() - 1);
    return toBlockAddress(candidateLines[lineDistribution(rng)]);
}

float computeLength(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

float computeRamRouteLength(const view::RamView::ReadPath& readPath) {
    const float laneDistance = computeLength(readPath.lanePosition - readPath.sourcePosition);
    const float toTurnEntryDistance = computeLength(readPath.turnEntryPosition - readPath.lanePosition);
    const float firstTurnDistance =
        std::abs(readPath.turnEndAngle - readPath.turnStartAngle) * readPath.turnRadius;
    const float collectorDistance = computeLength(readPath.collectorPosition - readPath.turnExitPosition);
    const float junctionTurnDistance =
        std::abs(readPath.junctionTurnEndAngle - readPath.junctionTurnStartAngle) * readPath.junctionTurnRadius;
    const float exitDistance = computeLength(readPath.exitPosition - readPath.junctionTurnExitPosition);
    return laneDistance + toTurnEntryDistance + firstTurnDistance + collectorDistance + junctionTurnDistance +
           exitDistance;
}

sim::Tick ticksForDistance(float distance, float pixelsPerTick, sim::Tick minimumTicks) {
    if (distance <= 0.0f) {
        return minimumTicks;
    }

    return std::max(minimumTicks,
                    static_cast<sim::Tick>(std::ceil(distance / std::max(pixelsPerTick, 1.0f))));
}

sim::MemoryTransactionDurations computeVisualLoadDurations(const view::RamView::ReadPath& readPath,
                                                           const view::rails::RailPath& busPath,
                                                           const view::rails::RailPath& installPath) {
    return {ticksForDistance(computeRamRouteLength(readPath), kPixelsPerTick, kMinToRamPortTicks),
            ticksForDistance(busPath.getLength(), kPixelsPerTick, kMinBusTicks),
            ticksForDistance(installPath.getLength(), kPixelsPerTick, kMinInstallTicks)};
}

const sim::MemoryTransaction* findActiveTransaction(const sim::Simulation& simulation) {
    for (auto it = simulation.getTransactions().rbegin(); it != simulation.getTransactions().rend(); ++it) {
        if (!it->isCompleted(simulation.getCurrentTick())) {
            return &(*it);
        }
    }

    return nullptr;
}

void seedDemoStruct(sim::RAM& ram) {
    constexpr sim::Address kStructStartAddress = 0;
    constexpr std::size_t kStructFloatCount = 6;
    const std::vector<std::string> fieldNames{
        "x",
        "y",
        "z",
        "vx",
        "vy",
        "vz",
    };

    const std::size_t structCount = ram.getSizeInBytes() / (kStructFloatCount * sim::RAM::kFloatSizeInBytes);
    for (std::size_t structIndex = 0; structIndex < structCount; ++structIndex) {
        const sim::Address structAddress = kStructStartAddress + static_cast<sim::Address>(
                                                                    structIndex * kStructFloatCount *
                                                                    sim::RAM::kFloatSizeInBytes);

        const float base = static_cast<float>(structIndex);
        const std::vector<float> values{
            base * 1.5f + 1.0f,
            base * 1.5f + 2.0f,
            base * 1.5f + 3.0f,
            0.15f + base * 0.01f,
            0.25f + base * 0.01f,
            0.35f + base * 0.01f,
        };

        std::vector<std::string> labels;
        labels.reserve(kStructFloatCount);
        for (const std::string& fieldName : fieldNames) {
            labels.push_back(fieldName + "[" + std::to_string(structIndex) + "]");
        }

        ram.writeFloatArray(structAddress, values);
        ram.setFloatLabels(structAddress, labels);
    }
}
} // namespace

int main() {
    constexpr const char* kWindowTitle = "CoreLab - CPU Visualizer Prototype";
    sf::ContextSettings contextSettings;
    contextSettings.antiAliasingLevel = 8;

    bool isFullscreen = false;
    sf::RenderWindow window;

    sf::View sceneView(kInitialCameraPosition,
                       {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    Camera camera(sceneView, 650.0f, 0.1f);
    camera.reset(kInitialCameraPosition, kInitialZoom);

    const auto applyWindowMode = [&window, &sceneView, &camera, contextSettings, &isFullscreen]() {
        const sf::VideoMode videoMode = isFullscreen ? sf::VideoMode::getDesktopMode()
                                                     : sf::VideoMode({kWindowWidth, kWindowHeight});
        const auto style = isFullscreen ? sf::Style::None : (sf::Style::Titlebar | sf::Style::Close);
        const sf::State state = isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;
        window.create(videoMode, kWindowTitle, style, state, contextSettings);
        window.setVerticalSyncEnabled(true);
        sceneView.setCenter(camera.getPosition());
        sceneView.setSize(
            {static_cast<float>(window.getSize().x) / camera.getZoom(),
             static_cast<float>(window.getSize().y) / camera.getZoom()});
    };

    applyWindowMode();

    // sim::Simulation simulation(4096+128);
    sim::Simulation simulation(64*6);
    seedDemoStruct(simulation.getRam());
    Scene scene;
    view::RamView ram(simulation.getRam().getSizeInBytes(), scene.getFont(), {0.0f, 0.0f});
    view::CpuView cpu(scene.getFont(), {-3400.0f, -700.0f});
    view::BusView ramToCacheBus(kRailThickness);
    ram.sync(simulation.getRam());
    cpu.syncPrimaryCache(simulation.getCache(), &simulation.getRam());
    MemoryReadAnimation readAnimation(scene.getFont());
    float simulationTickAccumulator = 0.0f;
    bool previousLeftMousePressed = false;
    DragTarget dragTarget = DragTarget::None;
    sf::Vector2f dragOffset{0.0f, 0.0f};
    std::mt19937 rng(std::random_device{}());

    sf::Clock frameClock;

    while (window.isOpen()) {
        const float deltaSeconds = frameClock.restart().asSeconds();
        const Controls::EventActions actions =
            Controls::handleEvents(window, camera, sceneView, kInitialCameraPosition, kInitialZoom);

        if (actions.requestExit) {
            window.close();
            continue;
        }

        if (actions.toggleFullscreen) {
            isFullscreen = !isFullscreen;
            applyWindowMode();
            previousLeftMousePressed = false;
            dragTarget = DragTarget::None;
        }

        sf::Vector2f movement = Controls::readMovement();

        if (movement != sf::Vector2f(0.0f, 0.0f)) {
            movement = normalizeOrZero(movement);
            camera.move(movement * camera.getSpeed() * deltaSeconds);
        }

        const bool leftMousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        const sf::Vector2f mouseWorldPosition = camera.screenToWorld(sf::Mouse::getPosition(window));
        const bool ramHovered = ram.isInDragHandle(mouseWorldPosition);
        const bool cpuHovered = cpu.isInDragHandle(mouseWorldPosition);

        if (leftMousePressed && !previousLeftMousePressed) {
            if (cpuHovered) {
                dragTarget = DragTarget::Cpu;
                dragOffset = mouseWorldPosition - cpu.getPosition();
            } else if (ramHovered) {
                dragTarget = DragTarget::Ram;
                dragOffset = mouseWorldPosition - ram.getPosition();
            }
        }

        if (!leftMousePressed) {
            dragTarget = DragTarget::None;
        } else if (dragTarget == DragTarget::Ram) {
            ram.setPosition(mouseWorldPosition - dragOffset);
        } else if (dragTarget == DragTarget::Cpu) {
            cpu.setPosition(mouseWorldPosition - dragOffset);
        }

        ram.setDragState(ramHovered || dragTarget == DragTarget::Ram, dragTarget == DragTarget::Ram);
        cpu.setDragState(cpuHovered || dragTarget == DragTarget::Cpu, dragTarget == DragTarget::Cpu);

        previousLeftMousePressed = leftMousePressed;

        simulationTickAccumulator += deltaSeconds * kSimulationTicksPerSecond;
        while (simulationTickAccumulator >= 1.0f) {
            simulation.advance(1);
            simulationTickAccumulator -= 1.0f;
        }

        const sim::MemoryTransaction* activeTransaction = findActiveTransaction(simulation);
        if (!activeTransaction) {
            const sim::Address address = chooseRandomAnimatedAddress(simulation, rng);
            const std::size_t lineIndex = static_cast<std::size_t>(address / sim::RAM::kCacheLineSizeInBytes);
            const std::size_t targetCacheSlotIndex = simulation.getCache().getTargetSlotIndex(address);
            const view::RamView::ReadPath previewReadPath = ram.getReadPath(lineIndex);
            ram.sync(simulation.getRam());
            cpu.syncPrimaryCache(simulation.getCache(), &simulation.getRam());
            view::CacheView* primaryCache = cpu.getPrimaryCacheView();
            view::BusView previewBus(kRailThickness);
            if (primaryCache) {
                if (const view::PortView* ramOut = ram.findPort("mem_out")) {
                    if (const view::PortView* cacheIn = primaryCache->findPort("mem_in")) {
                        previewBus.connect(*ramOut, *cacheIn);
                    }
                }
            }

            const sim::MemoryTransactionDurations visualDurations = primaryCache
                                                                        ? computeVisualLoadDurations(
                                                                              previewReadPath,
                                                                              previewBus.getPath(),
                                                                              primaryCache->getInstallPath(
                                                                                  targetCacheSlotIndex))
                                                                        : sim::MemoryTransactionDurations{};

            simulation.loadFloat(address, visualDurations);
            activeTransaction = findActiveTransaction(simulation);
        }

        view::CacheView* primaryCache = cpu.getPrimaryCacheView();

        if (activeTransaction && primaryCache) {
            const std::size_t lineIndex = static_cast<std::size_t>(activeTransaction->getLineBaseAddress() /
                                                                   sim::RAM::kCacheLineSizeInBytes);
            const view::RamView::ReadPath readPath = ram.getReadPath(lineIndex);
            ram.sync(simulation.getRam());
            cpu.syncPrimaryCache(simulation.getCache(), &simulation.getRam(), activeTransaction);
            primaryCache = cpu.getPrimaryCacheView();
            ram.setHighlightedLine(lineIndex);

            if (primaryCache) {
                primaryCache->setHighlightedSlot(activeTransaction->getTargetCacheSlotIndex());
                if (const view::PortView* ramOut = ram.findPort("mem_out")) {
                    if (const view::PortView* cacheIn = primaryCache->findPort("mem_in")) {
                        ramToCacheBus.connect(*ramOut, *cacheIn);
                    }
                }
                ramToCacheBus.setHighlighted(true);
                readAnimation.setCellLabels(simulation.getRam().getLineCellLabels(lineIndex));
                readAnimation.setRoute(readPath.sourcePosition,
                                       readPath.lanePosition,
                                       readPath.turnEntryPosition,
                                       readPath.turnCenter,
                                       readPath.turnRadius,
                                       readPath.turnStartAngle,
                                       readPath.turnEndAngle,
                                       readPath.turnExitPosition,
                                       readPath.collectorPosition,
                                       readPath.junctionTurnCenter,
                                       readPath.junctionTurnRadius,
                                       readPath.junctionTurnStartAngle,
                                       readPath.junctionTurnEndAngle,
                                       readPath.junctionTurnExitPosition,
                                       readPath.exitPosition,
                                       primaryCache->getLineHeadCenter());
                readAnimation.sync(
                    *activeTransaction,
                    simulation.getCurrentTick(),
                    ramToCacheBus.isVisible() ? &ramToCacheBus.getPath() : nullptr,
                    &primaryCache->getInstallPath());
            }
        } else {
            ram.sync(simulation.getRam());
            cpu.syncPrimaryCache(simulation.getCache(), &simulation.getRam());
            ram.setHighlightedLine(std::nullopt);
            if (primaryCache) {
                primaryCache->setHighlightedSlot(std::nullopt);
            }
            ramToCacheBus.clear();
            ramToCacheBus.setHighlighted(false);
            readAnimation.clear();
        }

        camera.update(window);

        window.clear(kBackgroundColor);
        window.setView(camera.getView());
        window.draw(scene);
        window.draw(cpu);
        window.draw(ram);
        window.draw(ramToCacheBus);
        window.draw(readAnimation);
        window.display();
    }

    return 0;
}
