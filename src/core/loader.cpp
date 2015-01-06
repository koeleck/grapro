#include <boost/tokenizer.hpp>

#include "loader.h"
#include "import/import.h"
#include "camera_manager.h"
#include "texture_manager.h"
#include "material_manager.h"
#include "mesh_manager.h"
#include "instance_manager.h"

#include "log/log.h"

namespace core
{

/****************************************************************************/

bool loadScenefiles(const std::string& scenefiles)
{
    bool result = true;
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tokens(scenefiles, sep);
    for (const auto& file : tokens) {
        auto scene = import::importSceneFile(file);
        if (!scene) {
            result = false;
            continue;
        }

        for (unsigned int i = 0; i < scene->num_cameras; i++) {
            const auto* cam = scene->cameras[i];
            res::cameras->createPerspectiveCam(cam->name, cam->position,
                    cam->position + cam->direction,
                    2.0 * glm::atan(glm::tan(glm::radians(cam->hfov) / 2.0) / cam->aspect_ratio),
                    cam->aspect_ratio, 0.1);
        }
        for (unsigned int i = 0; i < scene->num_textures; ++i) {
            const auto* tex = scene->textures[i];
            res::textures->addTexture(tex->name, *tex->image);
            //LOG_INFO("Added texture: ", tex->name);
        }
        for (unsigned int i = 0; i < scene->num_materials; ++i) {
            const auto* mat = scene->materials[i];
            res::materials->addMaterial(mat->name, mat, scene->textures);
            //LOG_INFO("Added material: ", mat->name);
        }
        for (unsigned int i = 0; i < scene->num_meshes; ++i) {
            const auto* mesh = scene->meshes[i];
            res::meshes->addMesh(mesh);
            //LOG_INFO("Added mesh: ", mesh->name);
        }
        for (unsigned int i = 0; i < scene->num_nodes; ++i) {
            const auto* node = scene->nodes[i];
            const auto* mesh = scene->meshes[node->mesh_index];
            const auto* mat = scene->materials[mesh->material_index];
            res::instances->addInstance(node->name,
                    res::meshes->getMesh(mesh->name),
                    res::materials->getMaterial(mat->name));
            //LOG_INFO("Added instance: ", node->name);
        }
        for (unsigned int i = 0; i < scene->num_lights; ++i) {
            // TODO
        }
    }

    if (res::cameras->getDefaultCam() == nullptr) {
        auto* cam = res::cameras->createPerspectiveCam("default_cam", glm::dvec3(0.0),
                glm::dvec3(0.0, 0.0, 1.0), glm::radians(45.0), 4.0 / 3.0, 0.1);
        res::cameras->makeDefault(cam);
    }

    return result;
}

/****************************************************************************/

} // namespace core

