#include <limits>
#include <glm/gtc/constants.hpp>
#include "light.h"
#include "shader_interface.h"

namespace core
{

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
    m_max_distance{std::numeric_limits<float>::infinity()},
    m_type{type},
    m_depth_tex{depthTex}
{
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
    // TODO
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
    // TODO
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
    // TODO
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
    // TODO
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
    // TODO
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
    // TODO
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
    m_angle_inner_cone{glm::pi<float>()},
    m_angle_outer_cone{glm::pi<float>()}
{
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
    // TODO
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
    // TODO
}

/****************************************************************************/

const glm::vec3& SpotLight::getDirection() const
{
    return m_direction;
}

/****************************************************************************/

void SpotLight::setDirection(const glm::vec3& dir)
{
    m_direction = dir;
    // TODO
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
    // TODO
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
    // TODO
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
    // TODO
}

/*****************************************************************************
 *
 * PointLight
 *
 ****************************************************************************/

PointLight::PointLight(shader::LightStruct* const data, const int depthTex)
  : Light(data, LightType::POINT, depthTex)
{
}

/****************************************************************************/

} // namespace core

