#pragma once

#include <SFML/Graphics.hpp>

#include "MachineController.h"

class App {
  public:
    App();
    int run();

  private:
    void applyWindowMode();

    static constexpr unsigned int kWindowWidth = 1600;
    static constexpr unsigned int kWindowHeight = 900;
    static constexpr float kInitialZoom = 0.5f;
    static constexpr sf::Vector2f kInitialCameraPosition{0.0f, 0.0f};

    bool isFullscreen = false;
    sf::ContextSettings contextSettings;
    sf::RenderWindow window;
    MachineController machineController;
};
