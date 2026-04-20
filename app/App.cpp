#include "App.h"

namespace {
constexpr const char* kWindowTitle = "CoreLab - CPU Visualizer Prototype";
} // namespace

App::App() {
    contextSettings.antiAliasingLevel = 8;
    applyWindowMode();
}

int App::run() {
    machineController.setAnimationTiming({
        .simulationTicksPerSecond = 60.0f,
        .pixelsPerTick = 34.0f,
        .minToRamPortTicks = 16,
        .minBusTicks = 34,
        .minInstallTicks = 22,
    });

    machineController.clearScene();
    machineController.createRam("ram0", 64 * 24, {0.0f, 0.0f});
    machineController.createCpu("cpu0", {-3400.0f, -700.0f});
    machineController.connect("ram0_to_cpu0_l1", "ram0", "mem_out", "cpu0.l1", "mem_in");
    constexpr auto labelMode = MachineController::LabelMode::FieldNames;
    machineController.writeStructArray("ram0", {"x", "y", "z", "vx", "vy", "vz"}, 16, labelMode);
    machineController.refreshVisualState();
    sf::Clock frameClock;

    while (window.isOpen()) {
        const float deltaSeconds = frameClock.restart().asSeconds();
        const Controls::EventActions actions = machineController.handleEvents(window);

        if (actions.requestExit) {
            window.close();
            continue;
        }

        if (actions.toggleFullscreen) {
            isFullscreen = !isFullscreen;
            applyWindowMode();
            machineController.cancelInteraction();
        }

        machineController.update(window, deltaSeconds);
        machineController.render(window);
        window.display();
    }

    return 0;
}

void App::applyWindowMode() {
    const sf::VideoMode videoMode =
        isFullscreen ? sf::VideoMode::getDesktopMode() : sf::VideoMode({kWindowWidth, kWindowHeight});
    const auto style = isFullscreen ? sf::Style::None : (sf::Style::Titlebar | sf::Style::Close);
    const sf::State state = isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;
    window.create(videoMode, kWindowTitle, style, state, contextSettings);
    window.setVerticalSyncEnabled(true);
    machineController.onWindowModeChanged(window);
}
