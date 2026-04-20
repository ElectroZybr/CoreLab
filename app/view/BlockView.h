#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "view/PortView.h"
#include "view/ViewNode.h"

namespace view {
class BlockView : public ViewNode {
  public:
    struct HeaderLayout {
        float dragHandleHeight = 122.0f;
        float titleY = 18.0f;
        float subtitleY = 98.0f;
        unsigned int titleTextSize = 74;
        unsigned int subtitleTextSize = 18;
    };

    explicit BlockView(const sf::Font* font = nullptr,
                       sf::Vector2f position = {0.0f, 0.0f},
                       sf::Vector2f size = {0.0f, 0.0f});

    void setPosition(sf::Vector2f position) override;
    [[nodiscard]] sf::FloatRect getBounds() const;
    [[nodiscard]] bool isInDragHandle(sf::Vector2f worldPoint) const;
    void setDragState(bool hovered, bool dragging);

    void setFont(const sf::Font* font);
    [[nodiscard]] const sf::Font* getFont() const {
        return m_font;
    }

    PortView& addPort(std::string id,
                      PortKind kind = PortKind::Bidirectional,
                      PortDirection direction = PortDirection::Right,
                      PayloadKind payloadKind = PayloadKind::Generic);
    PortView* findPort(std::string_view id);
    const PortView* findPort(std::string_view id) const;
    [[nodiscard]] const std::vector<std::unique_ptr<PortView>>& getPorts() const {
        return m_ports;
    }

  protected:
    static void buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius);

    void setBlockSize(sf::Vector2f size);
    [[nodiscard]] sf::Vector2f getBlockSize() const {
        return m_size;
    }

    void setTitle(std::string title);
    void setSubtitle(std::string subtitle);
    void setHeaderLayout(HeaderLayout headerLayout);
    [[nodiscard]] HeaderLayout getHeaderLayout() const {
        return m_headerLayout;
    }

    virtual void rebuildBlockGeometry();
    virtual void rebuildBlockText();
    virtual void layoutBlock();
    void onPositionChanged() override;
    virtual void drawBlockContent(sf::RenderTarget& target, sf::RenderStates states) const = 0;
    virtual void drawBlockOverlay(sf::RenderTarget& target, sf::RenderStates states) const {
    }

    const sf::Color m_containerFillColor{36, 44, 60};
    const sf::Color m_containerOutlineColor{88, 112, 150};
    const sf::Color m_titleColor{233, 245, 255};
    const sf::Color m_subtitleColor{182, 203, 229};
    const sf::Color m_dragHandleHoverColor{148, 172, 210, 34};
    const sf::Color m_dragHandleActiveColor{186, 214, 255, 54};
    float m_cornerRadius = 18.0f;

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    const sf::Font* m_font = nullptr;
    sf::Vector2f m_size{0.0f, 0.0f};
    HeaderLayout m_headerLayout;
    std::string m_title;
    std::string m_subtitle;
    sf::ConvexShape m_container;
    sf::ConvexShape m_dragHandleOverlay;
    bool m_dragHovered = false;
    bool m_dragging = false;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_subtitleText;
    std::vector<std::unique_ptr<PortView>> m_ports;
};
} // namespace view
