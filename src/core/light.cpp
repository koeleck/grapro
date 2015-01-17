#include "light.h"

namespace core
{

/*****************************************************************************
 *
 * Light base class
 *
 ****************************************************************************/

Light::Light() = default;

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

/****************************************************************************/

} // namespace core
