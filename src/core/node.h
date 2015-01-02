#ifndef CORE_NODE_H
#define CORE_NODE_H

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core
{

class Node
{
public:
    Node(Node* parent);

    virtual ~Node() = default;

    const glm::mat4& getTransformationMatrix() const;

    void move(const glm::vec3& dir);

    void setPosition(const glm::vec3& pos);
    const glm::vec3& getPosition() const;

    void setOrientation(const glm::quat& orientation);
    const glm::quat& getOrientation() const;

    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;

    void rotate(const glm::vec3& axis, float angle);
    void yaw(float angle);
    void roll(float angle);
    void pitch(float angle);

    void setFixedYawAxis(const glm::vec3* axis);
    bool hasFixedYawAxis() const;
    const glm::vec3& getFixedYawAxis() const;

    bool modified() const;
    void setModified();

    void update();

    Node* getParent() const;

protected:
    virtual void update_impl();

    glm::mat4       m_transformation;

private:

    glm::vec3       m_position;
    glm::quat       m_orientation;
    glm::vec3       m_fixedYawAxis;
    bool            m_hasFixedYawAxis;
    bool            m_modified;
    Node*           m_parent;
};

} // namespace core

#endif // CORE_NODE_H

