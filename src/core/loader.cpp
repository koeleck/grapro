#include <algorithm>
#include <boost/tokenizer.hpp>

#include "loader.h"
#include "import/import.h"
#include "camera_manager.h"
#include "texture_manager.h"
#include "material_manager.h"
#include "mesh_manager.h"
#include "instance_manager.h"
#include "aabb.h"

#include "log/log.h"

namespace core
{

/****************************************************************************/

bool loadScenefiles(const std::string& scenefiles)
{
    constexpr double NEAR_PLANE = 0.1;
    constexpr double FAR_PLANE = std::numeric_limits<double>::infinity();

    AABB scene_bbox;
    bool result = true;
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tokens(scenefiles, sep);
    for (const auto& file : tokens) {
        auto scene = import::importSceneFile(file);
        if (!scene) {
            result = false;
            continue;
        }

        for (unsigned int i = 0; i < scene->num_textures; ++i) {
            const auto* tex = scene->textures[i];
            res::textures->addTexture(tex->name, *tex->image);
        }
        for (unsigned int i = 0; i < scene->num_materials; ++i) {
            const auto* mat = scene->materials[i];
            res::materials->addMaterial(mat->name, mat, scene->textures);
        }
        for (unsigned int i = 0; i < scene->num_meshes; ++i) {
            const auto* mesh = scene->meshes[i];
            res::meshes->addMesh(mesh);
        }

        for (unsigned int i = 0; i < scene->num_nodes; ++i) {
            const auto* node = scene->nodes[i];
            const auto* mesh = scene->meshes[node->mesh_index];
            const auto* mat = scene->materials[mesh->material_index];
            auto* inst = res::instances->addInstance(node->name,
                    res::meshes->getMesh(mesh->name),
                    res::materials->getMaterial(mat->name));

            inst->move(node->position);
            inst->setScale(node->scale);
            inst->setOrientation(node->rotation);
            scene_bbox.expandBy(inst->getBoundingBox());
        }

        for (unsigned int i = 0; i < scene->num_cameras; i++) {
            const auto* cam = scene->cameras[i];
            res::cameras->createPerspectiveCam(cam->name, cam->position,
                    cam->position + cam->direction,
                    2.0 * glm::atan(glm::tan(glm::radians(cam->hfov) / 2.0) / cam->aspect_ratio),
                    cam->aspect_ratio, NEAR_PLANE, FAR_PLANE);
        }

        for (unsigned int i = 0; i < scene->num_lights; ++i) {
            // TODO
        }
    }

    if (res::cameras->getDefaultCam() == nullptr) {
        auto* cam = res::cameras->createPerspectiveCam("default_cam", glm::dvec3(-5000.0, 5000.0, 0.0),
                glm::dvec3(1.0, -1.0, 0.0), glm::radians(45.0), 4.0 / 3.0,
                NEAR_PLANE, FAR_PLANE);
        glm::dvec3 up(0.0, 1.0, 0.0);
        cam->setFixedYawAxis(true, up);
        cam->lookAt(glm::dvec3(0.0));
        res::cameras->makeDefault(cam);
    }

    return result;
}

/****************************************************************************/

} // namespace core

