#include "Scene.h"

#include <filesystem>

namespace {
bool loadFont(sf::Font& font) {
    const std::filesystem::path localFont = "assets/fonts/arial.ttf";
    if (std::filesystem::exists(localFont) && font.openFromFile(localFont)) {
        return true;
    }

    const std::filesystem::path windowsFont = "C:/Windows/Fonts/arial.ttf";
    return std::filesystem::exists(windowsFont) && font.openFromFile(windowsFont);
}
} // namespace

Scene::Scene() : m_grid(sf::PrimitiveType::Lines) {
    m_hasFont = loadFont(m_font);
    buildGrid();
}

void Scene::buildGrid() {
    constexpr int gridExtent = 5000;
    constexpr int gridStep = 250;
    const sf::Color color(40, 48, 68);

    for (int x = -gridExtent; x <= gridExtent; x += gridStep) {
        m_grid.append(sf::Vertex({static_cast<float>(x), static_cast<float>(-gridExtent)}, color));
        m_grid.append(sf::Vertex({static_cast<float>(x), static_cast<float>(gridExtent)}, color));
    }

    for (int y = -gridExtent; y <= gridExtent; y += gridStep) {
        m_grid.append(sf::Vertex({static_cast<float>(-gridExtent), static_cast<float>(y)}, color));
        m_grid.append(sf::Vertex({static_cast<float>(gridExtent), static_cast<float>(y)}, color));
    }
}

void Scene::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(m_grid, states);
}
