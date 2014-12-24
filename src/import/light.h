#ifndef IMPORT_LIGHT_H
#define IMPORT_LIGHT_H

#include <glm/glm.hpp>

namespace import
{

enum class LightType : char
{
    POINT,
    SPOT,
    DIRECTIONAL
};

struct Light
{
    static constexpr int VERSION = 1;

    void makePointersRelative();
    void makePointersAbsolute();

    const char* name;
    glm::vec3   position;
    glm::vec3   direction;
    glm::vec3   color;
    float       angle_inner_cone;
    float       angle_outer_cone;
    float       constant_attenuation;
    float       linear_attenuation;
    float       quadratic_attenuation;
    float       max_distance;
    LightType   type;
};

} // namespace import

#endif // IMPORT_LIGHT_H

