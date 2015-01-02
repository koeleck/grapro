#include "node.h"

namespace core
{

/****************************************************************************/

Node::Node(Node* parent)
  : m_transformation{},
    m_position{.0f},
    m_orientation{},
    m_fixedYawAxis{false},
    m_hasFixedYawAxis{},
    m_modified{true},
    m_parent{parent}
{
}

/****************************************************************************/

const glm::mat4& Node::getTransformationMatrix() const
{
    return m_transformation;
}

/****************************************************************************/

void Node::move(const glm::vec3& dir)
{
    m_position += dir;
    setModified();
}

/****************************************************************************/

void Node::setPosition(const glm::vec3& pos)
{
    m_position = pos;
    setModified();
}

/****************************************************************************/

const glm::vec3& Node::getPosition() const
{
    return m_position;
}

/****************************************************************************/

void Node::setOrientation(const glm::quat& orientation)
{
    m_orientation = glm::normalize(orientation);
    setModified();
}

/****************************************************************************/

const glm::quat& Node::getOrientation() const
{
    return m_orientation;
}

/****************************************************************************/

glm::vec3 Node::getForward() const
{
    return m_orientation * glm::vec3(.0f, .0f, -1.f); // ???
}

/****************************************************************************/

glm::vec3 Node::getRight() const
{
    return m_orientation * glm::vec3(1.f, .0f, .0f);
}

/****************************************************************************/

glm::vec3 Node::getUp() const
{
    return m_orientation * glm::vec3(.0f, 1.f, .0f);
}

/****************************************************************************/

void Node::rotate(const glm::vec3& axis, float angle)
{
    glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
    m_orientation = glm::normalize(m_orientation * q);
    setModified();
}

/****************************************************************************/

void Node::yaw(float angle)
{
    if (m_hasFixedYawAxis) {
        m_orientation = glm::normalize(glm::angleAxis(angle, m_fixedYawAxis) * m_orientation);
    } else {
        m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(.0f, 1.f, .0f)));
    }
    setModified();
}

/****************************************************************************/

void Node::roll(float angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(.0f, .0f, 1.f)));
    setModified();
}

/****************************************************************************/

void Node::pitch(float angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(1.f, .0f, .0f)));
    setModified();
}

/****************************************************************************/

void Node::setFixedYawAxis(const glm::vec3* axis)
{
    if (axis == nullptr) {
        m_hasFixedYawAxis = false;
    } else {
        m_fixedYawAxis = glm::normalize(*axis);
        m_hasFixedYawAxis = true;
    }
}

/****************************************************************************/

bool Node::hasFixedYawAxis() const
{
    return m_hasFixedYawAxis;
}

/****************************************************************************/

const glm::vec3& Node::getFixedYawAxis() const
{
    return m_fixedYawAxis;
}

/****************************************************************************/

bool Node::modified() const
{
    return m_modified;
}

/****************************************************************************/

void Node::setModified()
{
    m_modified = true;
}

/****************************************************************************/

void Node::update()
{
    if (m_modified) {
        glm::mat4 translation;
        translation[3] = glm::vec4(m_position, 1.f);
        m_transformation = translation * glm::mat4_cast(m_orientation);

        update_impl();

        m_modified = false;
    }
}

/****************************************************************************/

void Node::update_impl()
{
}

/****************************************************************************/

Node* Node::getParent() const
{
    return m_parent;
}

/****************************************************************************/

} // namespace core
