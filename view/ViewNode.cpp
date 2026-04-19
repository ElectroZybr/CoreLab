#include "view/ViewNode.h"

#include <algorithm>
#include <functional>

namespace view {
void ViewNode::setPosition(sf::Vector2f position) {
    std::function<void(ViewNode&)> refreshSubtree = [&](ViewNode& node) {
        node.onPositionChanged();
        for (ViewNode* child : node.m_children) {
            if (child) {
                refreshSubtree(*child);
            }
        }
    };

    m_position = position;
    onPositionChanged();
    for (ViewNode* child : m_children) {
        if (child) {
            refreshSubtree(*child);
        }
    }
}

sf::Vector2f ViewNode::getWorldPosition() const {
    if (!m_parent) {
        return m_position;
    }

    return m_parent->getWorldPosition() + m_position;
}

void ViewNode::addChild(ViewNode& child) {
    std::function<void(ViewNode&)> refreshSubtree = [&](ViewNode& node) {
        node.onPositionChanged();
        for (ViewNode* nestedChild : node.m_children) {
            if (nestedChild) {
                refreshSubtree(*nestedChild);
            }
        }
    };

    if (std::find(m_children.begin(), m_children.end(), &child) != m_children.end()) {
        return;
    }

    child.m_parent = this;
    m_children.push_back(&child);
    refreshSubtree(child);
}

void ViewNode::removeChild(ViewNode& child) {
    const auto it = std::find(m_children.begin(), m_children.end(), &child);
    if (it == m_children.end()) {
        return;
    }

    child.m_parent = nullptr;
    m_children.erase(it);
}

void ViewNode::onPositionChanged() {
}

void ViewNode::drawChildren(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const ViewNode* child : m_children) {
        if (child) {
            target.draw(*child, states);
        }
    }
}
} // namespace view
