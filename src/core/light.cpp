#include <limits>
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "light.h"
#include "shader_interface.h"
#include "log/log.h"

namespace core
{

namespace
{
constexpr auto NEAR_PLANE = 5.f; // optimized for sponza
} // anonymous namespace

/*****************************************************************************
 *
 * Light base class
 *
 ****************************************************************************/

Light::Light(shader::LightStruct* const data, const LightType type,
        const int depthTex)
  : m_data{data},
    m_position{.0f},
    m_intensity{.0f},
    m_constant_attenuation{.0f},
    m_linear_attenuation{.0f},
    m_quadratic_attenuation{.0f},
    m_max_distance{10.f},
    m_type{type},
    m_depth_tex{depthTex}
{
    m_data->projViewMatrix = glm::mat4();
    m_data->position = m_position;
    m_data->intensity = m_intensity;
    m_data->maxDistance = m_max_distance;
    m_data->constantAttenuation = m_constant_attenuation;
    m_data->linearAttenuation = m_linear_attenuation;
    m_data->quadraticAttenuation = m_quadratic_attenuation;

    GLint type_texid = 0;
    switch (type) {
    case LightType::SPOT:
        type_texid = shader::LightStruct::Type::SPOT;
        break;
    case LightType::DIRECTIONAL:
        type_texid = shader::LightStruct::Type::DIRECTIONAL;
        break;
    case LightType::POINT:
        type_texid = shader::LightStruct::Type::POINT;
        break;
    default:
        abort();
    }
    if (depthTex != -1) {
        type_texid |= 0x80000000;
        assert(depthTex <= 0x0fffffff);
        type_texid |= depthTex;
    }
    m_data->type_texid = type_texid;
}

/****************************************************************************/

Light::~Light() = default;

/****************************************************************************/

const glm::vec3& Light::getPosition() const
{
    return m_position;
}

/****************************************************************************/

void Light::setPosition(const glm::vec3& position)
{
    m_position = position;
    m_data->position = position;
    updateMatrix();
}

/****************************************************************************/

const glm::vec3& Light::getIntensity() const
{
    return m_intensity;
}

/****************************************************************************/

void Light::setIntensity(const glm::vec3& intensity)
{
    m_intensity = intensity;
    m_data->intensity = intensity;
}

/****************************************************************************/

float Light::getMaxDistance() const
{
    return m_max_distance;
}

/****************************************************************************/

void Light::setMaxDistance(const float dist)
{
    m_max_distance = dist;
    m_data->maxDistance = dist;
    updateMatrix();
}

/****************************************************************************/

LightType Light::getType() const
{
    return m_type;
}

/****************************************************************************/

int Light::getDepthTexIndex() const
{
    return m_depth_tex;
}

/****************************************************************************/

float Light::getConstantAttenuation() const
{
    return m_constant_attenuation;
}

/****************************************************************************/

void Light::setConstantAttenuation(const float attenuation)
{
    m_constant_attenuation = attenuation;
    m_data->constantAttenuation = attenuation;
}

/****************************************************************************/

float Light::getLinearAttenuation() const
{
    return m_linear_attenuation;
}

/****************************************************************************/

void Light::setLinearAttenuation(const float attenuation)
{
    m_linear_attenuation = attenuation;
    m_data->linearAttenuation = attenuation;
}

/****************************************************************************/

float Light::getQuadraticAttenuation() const
{
    return m_quadratic_attenuation;
}

/****************************************************************************/

void Light::setQuadraticAttenuation(const float attenuation)
{
    m_quadratic_attenuation = attenuation;
    m_data->quadraticAttenuation = attenuation;
}

/****************************************************************************/

bool Light::isShadowcasting() const
{
    return m_depth_tex != -1;
}

/****************************************************************************/


/*****************************************************************************
 *
 * SpotLight
 *
 ****************************************************************************/

SpotLight::SpotLight(shader::LightStruct* const data, const int depthTex)
  : Light(data, LightType::SPOT, depthTex),
    m_direction{.0f, -1.f, .0f},
    m_angle_inner_cone{glm::pi<float>() / 2.f},
    m_angle_outer_cone{glm::pi<float>() / 2.f}
{
    m_data->direction = m_direction;
    m_data->angleInnerCone = std::cos(m_angle_inner_cone / 2.f);
    m_data->angleOuterCone = std::cos(m_angle_outer_cone / 2.f);

    updateMatrix();
}

/****************************************************************************/

float SpotLight::getAngleInnerCone() const
{
    return m_angle_inner_cone;
}

/****************************************************************************/

void SpotLight::setAngleInnerCone(const float angle)
{
    m_angle_inner_cone = angle;
    m_data->angleInnerCone = std::cos(angle / 2.f);
}

/****************************************************************************/

float SpotLight::getAngleOuterCone() const
{
    return m_angle_outer_cone;
}

/****************************************************************************/

void SpotLight::setAngleOuterCone(const float angle)
{
    m_angle_outer_cone = angle;
    m_data->angleOuterCone = std::cos(angle / 2.f);
}

/****************************************************************************/

const glm::vec3& SpotLight::getDirection() const
{
    return m_direction;
}

/****************************************************************************/

void SpotLight::setDirection(const glm::vec3& dir)
{
    m_direction = glm::normalize(dir);
    m_data->direction = m_direction;
    updateMatrix();
}

/****************************************************************************/

void SpotLight::updateMatrix()
{
    glm::dmat4 projmat;
    if (getMaxDistance() == std::numeric_limits<float>::infinity()) {
        projmat = glm::infinitePerspective<double>(getAngleOuterCone(), 1.f, NEAR_PLANE);
    } else {
        projmat = glm::perspective<double>(getAngleOuterCone(), 1.f, NEAR_PLANE,
                getMaxDistance());
    }
    glm::dmat4 viewmat = glm::lookAt<double, glm::defaultp>(getPosition(),
            getPosition() + getDirection(), glm::vec3(.0f, 1.f, .0f));

    m_data->projViewMatrix = glm::mat4(projmat * viewmat);
}

/****************************************************************************/


/*****************************************************************************
 *
 * DirectionalLight
 *
 ****************************************************************************/

DirectionalLight::DirectionalLight(shader::LightStruct* const data,
        const int depthTex)
  : Light(data, LightType::DIRECTIONAL, depthTex),
    m_direction{.0f, -1.f, .0f},
    m_rotation{.0f},
    m_size{1.f}
{
    m_data->direction = m_direction;

    updateMatrix();
}

/****************************************************************************/

const glm::vec3& DirectionalLight::getDirection() const
{
    return m_direction;
}

/****************************************************************************/

void DirectionalLight::setDirection(const glm::vec3& dir)
{
    m_direction = dir;
    m_data->direction = dir;
    updateMatrix();
}

/****************************************************************************/

float DirectionalLight::getRotation() const
{
    return m_rotation;
}

/****************************************************************************/

void DirectionalLight::setRotation(const float angle)
{
    m_rotation = angle;
    updateMatrix();
}

/****************************************************************************/

float DirectionalLight::getSize() const
{
    return m_size;
}

/****************************************************************************/

void DirectionalLight::setSize(const float size)
{
    m_size = size;
    updateMatrix();
}

/****************************************************************************/

void DirectionalLight::updateMatrix()
{
    if (getMaxDistance() == std::numeric_limits<float>::infinity()) {
        if (isShadowcasting()) {
            // insult user:
            LOG_ERROR("Shadowcasting directional light can't have max "
                    "distance at infinity! Clamping to 5000.0, you idiot!");
            setMaxDistance(5000.f); // this will call updateMatrix() again
        }
        return;
    }

    const double dist = m_size / 2.f;
    glm::dmat4 projmat = glm::ortho<double>(-dist, dist, -dist, dist,
            NEAR_PLANE, getMaxDistance());

    // TODO Rotation
    glm::dmat4 viewmat = glm::lookAt<double, glm::defaultp>(getPosition(),
            getPosition() + getDirection(), glm::vec3(1.0f, .0f, 0.f));


    m_data->projViewMatrix = glm::mat4(projmat * viewmat);

}

/*****************************************************************************
 *
 * PointLight
 *
 ****************************************************************************/

PointLight::PointLight(shader::LightStruct* const data, const int depthTex)
  : Light(data, LightType::POINT, depthTex)
{
    updateMatrix();
}

/****************************************************************************/

void PointLight::updateMatrix()
{
    double angle = glm::radians(90.0);
    glm::dmat4 projmat;
    if (getMaxDistance() == std::numeric_limits<float>::infinity()) {
        projmat = glm::infinitePerspective<double>(angle, 1.f, NEAR_PLANE);
    } else {
        projmat = glm::perspective<double>(angle, 1.f, NEAR_PLANE,
                getMaxDistance());
    }
    // view transformation will be done in the shaders
    m_data->projViewMatrix = glm::mat4(projmat);

    // we have to compute the fragments depth in the shader, so
    // store the necessary values in 'direction'
    //  to compute depth : [0, 1] from abs(z):
    // --> depth = direction.x + direction.y / abs(z);
    const double a = 0.5 - projmat[2][2] / 2.0;
    const double b = projmat[3][2] / 2.0;
    m_data->direction = glm::vec3(a, b, .0f);
}

/****************************************************************************/

} // namespace core

