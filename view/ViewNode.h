#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

namespace view {
class ViewNode : public sf::Drawable {
  public:
    virtual ~ViewNode() = default;

    virtual void setPosition(sf::Vector2f position);
    [[nodiscard]] sf::Vector2f getPosition() const {
        return m_position;
    }
    [[nodiscard]] sf::Vector2f getWorldPosition() const;

    void addChild(ViewNode& child);
    void removeChild(ViewNode& child);
    [[nodiscard]] const std::vector<ViewNode*>& getChildren() const {
        return m_children;
    }

  protected:
    virtual void onPositionChanged();
    void drawChildren(sf::RenderTarget& target, sf::RenderStates states) const;

  private:
    sf::Vector2f m_position{0.0f, 0.0f};
    ViewNode* m_parent = nullptr;
    std::vector<ViewNode*> m_children;
};
} // namespace view
