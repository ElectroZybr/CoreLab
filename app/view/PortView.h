#pragma once

#include <SFML/Graphics.hpp>

#include <string>

#include "view/ViewNode.h"

namespace view {
enum class PortDirection {
    Left,
    Right,
    Up,
    Down,
};

enum class PortKind {
    Input,
    Output,
    Bidirectional,
};

enum class PayloadKind {
    Generic,
    CacheLine,
    Scalar,
    Vector,
};

class PortView : public ViewNode {
  public:
    explicit PortView(std::string id = {});

    void setId(std::string id);
    [[nodiscard]] const std::string& getId() const {
        return m_id;
    }

    void setDirection(PortDirection direction);
    [[nodiscard]] PortDirection getDirection() const {
        return m_direction;
    }

    void setKind(PortKind kind);
    [[nodiscard]] PortKind getKind() const {
        return m_kind;
    }

    void setPayloadKind(PayloadKind payloadKind);
    [[nodiscard]] PayloadKind getPayloadKind() const {
        return m_payloadKind;
    }

    void setLocalAnchor(sf::Vector2f anchor);
    [[nodiscard]] sf::Vector2f getLocalAnchor() const {
        return getPosition();
    }
    [[nodiscard]] sf::Vector2f getWorldAnchor() const {
        return getWorldPosition();
    }

    void setSize(sf::Vector2f size);
    [[nodiscard]] sf::Vector2f getSize() const {
        return m_size;
    }

    void setColors(sf::Color fillColor, sf::Color outlineColor);

  private:
    static void buildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius);
    void rebuildGeometry();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::string m_id;
    PortDirection m_direction = PortDirection::Right;
    PortKind m_kind = PortKind::Bidirectional;
    PayloadKind m_payloadKind = PayloadKind::Generic;
    sf::Vector2f m_size{52.0f, 28.0f};
    sf::Color m_fillColor{48, 58, 78};
    sf::Color m_outlineColor{132, 154, 191};
    sf::ConvexShape m_shape;
};
} // namespace view
