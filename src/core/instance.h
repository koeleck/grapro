#ifndef CORE_INSTANCE_H
#define CORE_INSTANCE_H

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include "gl/gl_sys.h"

namespace core
{

// Forward declarations:
namespace shader
{
struct InstanceStruct;
} // namespace shader
class Mesh;
class Material;

class Instance
{
public:
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

    GLuint getIndex() const;

    const Mesh* getMesh() const;
    void setMesh(const Mesh* mesh);

    const Material* getMaterial() const;
    void setMaterial(const Material* material);

protected:
    friend class InstanceManager;
    Instance(const Mesh* mesh, const Material* material,
            GLuint index, GLvoid* data);

    virtual void update_impl();

    glm::mat4       m_transformation;

private:

    glm::vec3       m_position;
    glm::vec3       m_scale;
    glm::quat       m_orientation;
    glm::vec3       m_fixedYawAxis;
    bool            m_hasFixedYawAxis;
    bool            m_modified;
    const Mesh*     m_mesh;
    const Material* m_material;
    GLuint          m_index;
    shader::InstanceStruct*     m_data;
};

} // namespace core

#endif // CORE_INSTANCE_H

