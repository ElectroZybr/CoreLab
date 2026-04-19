#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "animation/MemoryReadAnimation.h"
#include "core/Camera.h"
#include "input/Controls.h"
#include "scene/Scene.h"
#include "sim/simulation.h"
#include "view/BlockView.h"
#include "view/BusView.h"
#include "view/CacheView.h"
#include "view/CpuView.h"
#include "view/RamView.h"

class MachineController {
  public:
    MachineController();

    view::RamView& createRam(const std::string& id, std::size_t sizeInBytes, sf::Vector2f position);
    view::CpuView& createCpu(const std::string& id, sf::Vector2f position);
    view::BusView& connect(const std::string& id,
                           std::string fromComponentId,
                           std::string fromPortId,
                           std::string toComponentId,
                           std::string toPortId);
    view::BlockView* findComponent(std::string_view id);
    const view::BlockView* findComponent(std::string_view id) const;
    view::BusView* findConnection(std::string_view id);
    const view::BusView* findConnection(std::string_view id) const;

    void clearScene();
    void seedDemoStruct();
    void refreshVisualState();
    void cancelInteraction();
    [[nodiscard]] Controls::EventActions handleEvents(sf::RenderWindow& window);
    void update(sf::RenderWindow& window, float deltaSeconds);
    void render(sf::RenderWindow& window) const;
    void onWindowModeChanged(const sf::RenderWindow& window);

  private:
    struct Connection {
        std::string id;
        std::string fromComponentId;
        std::string fromPortId;
        std::string toComponentId;
        std::string toPortId;
        view::BusView bus{kRailThickness};
        bool highlighted = false;
    };

    void rebuildComponentRegistry();
    void refreshConnections();
    view::RamView* getDemoRam();
    const view::RamView* getDemoRam() const;
    view::CpuView* getDemoCpu();
    const view::CpuView* getDemoCpu() const;
    view::CacheView* getDemoPrimaryCache();
    const view::CacheView* getDemoPrimaryCache() const;
    Connection* getDemoConnection();
    const Connection* getDemoConnection() const;

    enum class DragTargetKind {
        None,
        Block,
    };

    struct DragTarget {
        DragTargetKind kind = DragTargetKind::None;
        view::BlockView* block = nullptr;
    };

    void syncIdleState();
    void syncActiveTransaction(const sim::MemoryTransaction& activeTransaction);
    [[nodiscard]] const sim::MemoryTransaction* findActiveTransaction() const;
    [[nodiscard]] sim::Address chooseRandomAnimatedAddress();
    [[nodiscard]] static sim::Tick ticksForDistance(float distance, float pixelsPerTick, sim::Tick minimumTicks);
    [[nodiscard]] static float computeLength(sf::Vector2f vector);
    [[nodiscard]] static float computeRamRouteLength(const view::RamView::ReadPath& readPath);
    [[nodiscard]] static sim::MemoryTransactionDurations computeVisualLoadDurations(
        const view::RamView::ReadPath& readPath,
        const view::rails::RailPath& busPath,
        const view::rails::RailPath& installPath);

    static constexpr float kSimulationTicksPerSecond = 60.0f;
    static constexpr float kRailThickness = 6.0f;
    static constexpr float kPixelsPerTick = 55.0f;
    static constexpr sim::Tick kMinToRamPortTicks = 10;
    static constexpr sim::Tick kMinBusTicks = 18;
    static constexpr sim::Tick kMinInstallTicks = 12;
    static constexpr float kInitialZoom = 0.5f;
    static constexpr sf::Vector2f kInitialCameraPosition{0.0f, 0.0f};
    static constexpr const char* kDemoRamId = "ram0";
    static constexpr const char* kDemoCpuId = "cpu0";
    static constexpr const char* kDemoPrimaryCacheId = "cpu0.l1";
    static constexpr const char* kDemoConnectionId = "ram0_to_cpu0_l1";

    sf::View sceneView;
    Camera camera;
    Scene scene;
    sim::Simulation simulation;
    MemoryReadAnimation readAnimation;
    std::unordered_map<std::string, std::unique_ptr<view::RamView>> ramViews;
    std::unordered_map<std::string, std::unique_ptr<view::CpuView>> cpuViews;
    std::unordered_map<std::string, view::BlockView*> componentViewsById;
    std::vector<view::BlockView*> topLevelBlocks;
    std::vector<Connection> connections;
    float simulationTickAccumulator = 0.0f;
    bool previousLeftMousePressed = false;
    DragTarget dragTarget{};
    sf::Vector2f dragOffset{0.0f, 0.0f};
    std::mt19937 rng;
};
