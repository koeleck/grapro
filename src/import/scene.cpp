#include "scene.h"
#include "import.h"

#include "log/log.h"
#include <iostream>
namespace import
{

/***************************************************************************/

Scene::~Scene()
{
    for (unsigned int i = 0; i < num_textures; ++i) {
        delete textures[i]->image;
        textures[i]->image = nullptr;
    }
}

/***************************************************************************/

void Scene::makePointersRelative()
{
    if (static_cast<const void*>(name) < static_cast<const void*>(this))
        return;

    const char* offset = reinterpret_cast<char*>(this);

    for (std::uint32_t i = 0; i < num_materials; ++i) {
        materials[i]->makePointersRelative();
        materials[i] = reinterpret_cast<Material*>(reinterpret_cast<const char*>(materials[i]) -
                offset);
    }
    for (std::uint32_t i = 0; i < num_textures; ++i) {
        textures[i]->makePointersRelative();
        textures[i] = reinterpret_cast<Texture*>(reinterpret_cast<const char*>(textures[i]) -
                offset);
    }
    for (std::uint32_t i = 0; i < num_meshes; ++i) {
        meshes[i]->makePointersRelative();
        meshes[i] = reinterpret_cast<Mesh*>(reinterpret_cast<const char*>(meshes[i]) -
                offset);
    }
    for (std::uint32_t i = 0; i < num_lights; ++i) {
        lights[i]->makePointersRelative();
        lights[i] = reinterpret_cast<Light*>(reinterpret_cast<const char*>(lights[i]) -
                offset);
    }
    for (std::uint32_t i = 0; i < num_cameras; ++i) {
        cameras[i]->makePointersRelative();
        cameras[i] = reinterpret_cast<Camera*>(reinterpret_cast<const char*>(cameras[i]) -
                offset);
    }
    for (std::uint32_t i = 0; i < num_nodes; ++i) {
        nodes[i]->makePointersRelative();
        nodes[i] = reinterpret_cast<Node*>(reinterpret_cast<const char*>(nodes[i]) -
                offset);
    }

    name = reinterpret_cast<const char*>(reinterpret_cast<const char*>(name) -
            offset);
    if (materials != nullptr) {
        materials = reinterpret_cast<Material**>(reinterpret_cast<const char*>(materials) -
                offset);
    }
    if (meshes != nullptr) {
        meshes = reinterpret_cast<Mesh**>(reinterpret_cast<const char*>(meshes) -
                offset);
    }
    if (lights != nullptr) {
        lights = reinterpret_cast<Light**>(reinterpret_cast<const char*>(lights) -
                offset);
    }
    if (textures != nullptr) {
        textures = reinterpret_cast<Texture**>(reinterpret_cast<const char*>(textures) -
                offset);
    }
    if (cameras != nullptr) {
        cameras = reinterpret_cast<Camera**>(reinterpret_cast<const char*>(cameras) -
                offset);
    }
    if (nodes != nullptr) {
        nodes = reinterpret_cast<Node**>(reinterpret_cast<const char*>(nodes) -
                offset);
    }
}

/***************************************************************************/

void Scene::makePointersAbsolute()
{
    if (static_cast<void*>(materials) > static_cast<void*>(this))
        return;

    char* offset = reinterpret_cast<char*>(this);
    name = reinterpret_cast<const char*>(offset +       reinterpret_cast<std::size_t>(name));
    if (materials != nullptr)
        materials = reinterpret_cast<Material**>(offset +   reinterpret_cast<std::size_t>(materials));
    if (meshes != nullptr)
        meshes = reinterpret_cast<Mesh**>(offset +          reinterpret_cast<std::size_t>(meshes));
    if (lights != nullptr)
        lights = reinterpret_cast<Light**>(offset +         reinterpret_cast<std::size_t>(lights));
    if (textures != nullptr)
        textures = reinterpret_cast<Texture**>(offset +     reinterpret_cast<std::size_t>(textures));
    if (cameras != nullptr)
        cameras = reinterpret_cast<Camera**>(offset +       reinterpret_cast<std::size_t>(cameras));
    if (nodes != nullptr)
        nodes = reinterpret_cast<Node**>(offset +           reinterpret_cast<std::size_t>(nodes));

    for (std::uint32_t i = 0; i < num_materials; ++i) {
        materials[i] = reinterpret_cast<Material*>(offset + reinterpret_cast<std::size_t>(materials[i]));
        materials[i]->makePointersAbsolute();
    }
    for (std::uint32_t i = 0; i < num_textures; ++i) {
        textures[i] = reinterpret_cast<Texture*>(offset + reinterpret_cast<std::size_t>(textures[i]));
        textures[i]->makePointersAbsolute();
    }
    for (std::uint32_t i = 0; i < num_meshes; ++i) {
        meshes[i] = reinterpret_cast<Mesh*>(offset + reinterpret_cast<std::size_t>(meshes[i]));
        meshes[i]->makePointersAbsolute();
    }
    for (std::uint32_t i = 0; i < num_lights; ++i) {
        lights[i] = reinterpret_cast<Light*>(offset + reinterpret_cast<std::size_t>(lights[i]));
        lights[i]->makePointersAbsolute();
    }
    for (std::uint32_t i = 0; i < num_cameras; ++i) {
        cameras[i] = reinterpret_cast<Camera*>(offset + reinterpret_cast<std::size_t>(cameras[i]));
        cameras[i]->makePointersAbsolute();
    }
    for (std::uint32_t i = 0; i < num_nodes; ++i) {
        nodes[i] = reinterpret_cast<Node*>(offset + reinterpret_cast<std::size_t>(nodes[i]));
        nodes[i]->makePointersAbsolute();
    }
}

/***************************************************************************/

} // namespace import

