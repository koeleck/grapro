#ifndef IMPORT_NODE_H
#define IMPORT_NODE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace import
{

struct Node
{
    static constexpr int VERSION = 1;

    void makePointersRelative();
    void makePointersAbsolute();

    const char*     name;
    glm::vec3       position;
    glm::vec3       scale;
    glm::quat       rotation;
    std::uint32_t   mesh_index;
};

} // namespace import

#endif // IMPORT_NODE_H

