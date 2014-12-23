#ifndef IMPORT_SCENE_H
#define IMPORT_SCENE_H

#include <cstdint>

namespace import
{

struct Material;
struct Mesh;
struct Light;
struct Texture;
struct Camera;
struct Node;

struct Scene
{
    static constexpr int VERSION = 1;

    const char* name;

    ~Scene();

    std::uint32_t num_materials;
    std::uint32_t num_textures;
    std::uint32_t num_meshes;
    std::uint32_t num_lights;
    std::uint32_t num_cameras;
    std::uint32_t num_nodes;

    Material**  materials;
    Mesh**      meshes;
    Light**     lights;
    Texture**   textures;
    Camera**    cameras;
    Node**      nodes;
};

} // namespace import

#endif // IMPORT_SCENE_H

