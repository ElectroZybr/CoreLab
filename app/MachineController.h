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
    struct AnimationTiming {
        float simulationTicksPerSecond = 60.0f;
        float pixelsPerTick = 40.0f;
        sim::Tick minToRamPortTicks = 14;
        sim::Tick minBusTicks = 26;
        sim::Tick minInstallTicks = 18;
    };

    enum class LabelMode {
        FieldNames,
        SequentialIndices,
    };

    struct StructLayout {
        std::vector<std::string> fieldNames;
    };

    using StructValues = std::vector<float>;

    class StructArray {
      public:
        StructArray() = default;
        StructArray(StructLayout layout, std::size_t count);

        [[nodiscard]] const StructLayout& getLayout() const {
            return layout;
        }
        [[nodiscard]] const std::vector<StructValues>& getInstances() const {
            return instances;
        }
        [[nodiscard]] std::size_t size() const {
            return instances.size();
        }

        void set(std::size_t structIndex, std::size_t fieldIndex, float value);
        void set(std::size_t structIndex, std::string_view fieldName, float value);

      private:
        [[nodiscard]] std::size_t findFieldIndex(std::string_view fieldName) const;

        StructLayout layout;
        std::vector<StructValues> instances;
    };

    MachineController();
    void setAnimationTiming(AnimationTiming timing) {
        animationTiming = timing;
    }
    [[nodiscard]] const AnimationTiming& getAnimationTiming() const {
        return animationTiming;
    }

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
    static StructArray createStructArray(StructLayout layout, std::size_t count);
    void writeStructArray(std::string_view ramId,
                          std::initializer_list<std::string> fieldNames,
                          std::size_t count,
                          LabelMode labelMode = LabelMode::FieldNames,
                          sim::Address startAddress = 0);
    void writeStructArray(std::string_view ramId,
                          const StructLayout& layout,
                          const std::vector<StructValues>& instances,
                          LabelMode labelMode = LabelMode::FieldNames,
                          sim::Address startAddress = 0);
    void writeStructArray(std::string_view ramId,
                          const StructArray& array,
                          LabelMode labelMode = LabelMode::FieldNames,
                          sim::Address startAddress = 0);

    void clearScene();
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
    [[nodiscard]] sim::MemoryTransactionDurations computeVisualLoadDurations(
        const view::RamView::ReadPath& readPath,
        const view::rails::RailPath& busPath,
        const view::rails::RailPath& installPath);

    static constexpr float kRailThickness = 6.0f;
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
    AnimationTiming animationTiming{};
    std::unordered_map<std::string, std::unique_ptr<view::RamView>> ramViews;
    std::unordered_map<std::string, std::unique_ptr<view::CpuView>> cpuViews;
    std::unordered_map<std::string, view::BlockView*> componentViewsById;
    std::vector<view::BlockView*> topLevelBlocks;
    std::vector<Connection> connections;
    float simulationTickAccumulator = 0.0f;
    bool previousLeftMousePressed = false;
    DragTarget dragTarget{};
    sf::Vector2f dragOffset{0.0f, 0.0f};
    std::size_t nextReadLineIndex = 0;
    std::mt19937 rng;
};
