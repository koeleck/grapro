#ifndef CORE_INSTANCE_H
#define CORE_INSTANCE_H

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core
{

class Instance
{
public:
    Instance(Instance* parent);

    virtual ~Instance() = default;

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

    const glm::vec3& getScale() const;
    void setScale(const glm::vec3& scale);

    bool modified() const;
    void setModified();

    void update();


protected:
    virtual void update_impl();

    glm::mat4       m_transformation;

private:

    glm::vec3       m_position;
    glm::vec3       m_scale;
    glm::quat       m_orientation;
    glm::vec3       m_fixedYawAxis;
    bool            m_hasFixedYawAxis;
    bool            m_modified;
};

} // namespace core

#endif // CORE_INSTANCE_H

