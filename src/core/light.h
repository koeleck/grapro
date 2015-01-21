#ifndef CORE_LIGHT_H
#define CORE_LIGHT_H

#include <glm/vec3.hpp>

namespace core
{

/****************************************************************************/
// forward declarations:
namespace shader
{
struct LightStruct;
} // namespace shader

/****************************************************************************/

enum class LightType : char
{
    POINT,
    SPOT,
    DIRECTIONAL
};

/****************************************************************************/

class Light
{
public:
    virtual ~Light();

    bool isShadowcasting() const;

    const glm::vec3& getPosition() const;
    void setPosition(const glm::vec3& pos);

    const glm::vec3& getIntensity() const;
    void setIntensity(const glm::vec3& intensity);

    float getMaxDistance() const;
    void setMaxDistance(float dist);

    LightType getType() const;

    int getDepthTexIndex() const;

    float getConstantAttenuation() const;
    void setConstantAttenuation(float attenuation);

    float getLinearAttenuation() const;
    void setLinearAttenuation(float attenuation);

    float getQuadraticAttenuation() const;
    void setQuadraticAttenuation(float attenuation);

protected:
    Light(shader::LightStruct* data, LightType type, int depthTex);
    shader::LightStruct*    m_data;

    virtual void updateMatrix() = 0;

private:
    glm::vec3               m_position;
    glm::vec3               m_intensity;
    float                   m_constant_attenuation;
    float                   m_linear_attenuation;
    float                   m_quadratic_attenuation;
    float                   m_max_distance;
    LightType               m_type;
    int                     m_depth_tex;
};

class SpotLight
  : public Light
{
public:
    float getAngleInnerCone() const;
    void setAngleInnerCone(float angle);

    float getAngleOuterCone() const;
    void setAngleOuterCone(float angle);

    const glm::vec3& getDirection() const;
    void setDirection(const glm::vec3& dir);

protected:
    friend class LightManager;
    SpotLight(shader::LightStruct* data, int depthTex);

    virtual void updateMatrix() override;

private:
    glm::vec3               m_direction;
    float                   m_angle_inner_cone;
    float                   m_angle_outer_cone;
};

/****************************************************************************/

class DirectionalLight
  : public Light
{
public:
    const glm::vec3& getDirection() const;
    void setDirection(const glm::vec3& dir);

    float getRotation() const;
    void setRotation(float angle);

    float getSize() const;
    void setSize(float size);

protected:
    friend class LightManager;
    DirectionalLight(shader::LightStruct* data, int depthTex);

    virtual void updateMatrix() override;

private:
    glm::vec3               m_direction;
    float                   m_rotation;
    float                   m_size;
};

/****************************************************************************/

class PointLight
  : public Light
{
public:

protected:
    friend class LightManager;
    PointLight(shader::LightStruct* data, int depthTex);

    virtual void updateMatrix() override;

private:
};

/****************************************************************************/

} // namespace core

#endif // CORE_LIGHT_H

