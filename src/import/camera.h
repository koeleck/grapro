#ifndef IMPORT_CAMERA_H
#define IMPORT_CAMERA_H

#include <glm/glm.hpp>

namespace import
{

struct Camera
{
    static constexpr int VERSION = 1;

    const char* name;
    glm::vec3   position;
    glm::vec3   direction;
    glm::vec3   up;
    float       aspect_ratio;
    float       hfov;

    void makePointersRelative();
    void makePointersAbsolute();
};

} // namespace import

#endif // IMPORT_CAMERA_H

