#include "instance.h"
#include "shader_interface.h"
#include "mesh.h"
#include "material.h"

namespace core
{

/****************************************************************************/

Instance::Instance(const Mesh* mesh, const Material* material,
            GLuint index, GLvoid* data)
  : m_transformation{},
    m_position{.0f},
    m_scale{1.f},
    m_orientation{},
    m_fixedYawAxis{false},
    m_hasFixedYawAxis{},
    m_modified{true},
    m_mesh{mesh},
    m_material{material},
    m_index{index},
    m_data{static_cast<shader::InstanceStruct*>(data)}
{
    setModified();
}

/****************************************************************************/

const glm::mat4& Instance::getTransformationMatrix() const
{
    return m_transformation;
}

/****************************************************************************/

void Instance::move(const glm::vec3& dir)
{
    m_position += dir;
    setModified();
}

/****************************************************************************/

void Instance::setPosition(const glm::vec3& pos)
{
    m_position = pos;
    setModified();
}

/****************************************************************************/

const glm::vec3& Instance::getPosition() const
{
    return m_position;
}

/****************************************************************************/

void Instance::setOrientation(const glm::quat& orientation)
{
    m_orientation = glm::normalize(orientation);
    setModified();
}

/****************************************************************************/

const glm::quat& Instance::getOrientation() const
{
    return m_orientation;
}

/****************************************************************************/

glm::vec3 Instance::getForward() const
{
    return m_orientation * glm::vec3(.0f, .0f, -1.f); // ???
}

/****************************************************************************/

glm::vec3 Instance::getRight() const
{
    return m_orientation * glm::vec3(1.f, .0f, .0f);
}

/****************************************************************************/

glm::vec3 Instance::getUp() const
{
    return m_orientation * glm::vec3(.0f, 1.f, .0f);
}

/****************************************************************************/

void Instance::rotate(const glm::vec3& axis, float angle)
{
    glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
    m_orientation = glm::normalize(m_orientation * q);
    setModified();
}

/****************************************************************************/

void Instance::yaw(float angle)
{
    if (m_hasFixedYawAxis) {
        m_orientation = glm::normalize(glm::angleAxis(angle, m_fixedYawAxis) * m_orientation);
    } else {
        m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(.0f, 1.f, .0f)));
    }
    setModified();
}

/****************************************************************************/

void Instance::roll(float angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(.0f, .0f, 1.f)));
    setModified();
}

/****************************************************************************/

void Instance::pitch(float angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::vec3(1.f, .0f, .0f)));
    setModified();
}

/****************************************************************************/

const glm::vec3& Instance::getScale() const
{
    return m_scale;
}

/****************************************************************************/

void Instance::setScale(const glm::vec3& scale)
{
    m_scale = scale;
    setModified();
}

/****************************************************************************/

void Instance::setFixedYawAxis(const glm::vec3* axis)
{
    if (axis == nullptr) {
        m_hasFixedYawAxis = false;
    } else {
        m_fixedYawAxis = glm::normalize(*axis);
        m_hasFixedYawAxis = true;
    }
}

/****************************************************************************/

bool Instance::hasFixedYawAxis() const
{
    return m_hasFixedYawAxis;
}

/****************************************************************************/

const glm::vec3& Instance::getFixedYawAxis() const
{
    return m_fixedYawAxis;
}

/****************************************************************************/

bool Instance::modified() const
{
    return m_modified;
}

/****************************************************************************/

void Instance::setModified()
{
    m_modified = true;
}

/****************************************************************************/

void Instance::update()
{
    if (m_modified) {
        glm::mat4 translation;
        translation[3] = glm::vec4(m_position, 1.f);
        m_transformation = translation * glm::mat4_cast(m_orientation);
        m_transformation[0] *= m_scale.x;
        m_transformation[1] *= m_scale.y;
        m_transformation[2] *= m_scale.z;

        update_impl();

        m_data->modelMatrix = m_transformation;
        m_data->meshID = m_mesh->getIndex();
        m_data->materialID = m_material->getIndex();
        m_modified = false;
    }
}

/****************************************************************************/

void Instance::update_impl()
{
}

/****************************************************************************/

GLuint Instance::getIndex() const
{
    return m_index;
}

/****************************************************************************/

const Mesh* Instance::getMesh() const
{
    return m_mesh;
}

/****************************************************************************/

void Instance::setMesh(const Mesh* mesh)
{
    m_mesh = mesh;
    setModified();
}

/****************************************************************************/

const Material* Instance::getMaterial() const
{
    return m_material;
}

/****************************************************************************/

void Instance::setMaterial(const Material* material)
{
    m_material = material;
    setModified();
}

/****************************************************************************/

} // namespace core

