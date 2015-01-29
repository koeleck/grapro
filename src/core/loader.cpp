#include <algorithm>
#include <cstring>
#include <boost/tokenizer.hpp>

#include "loader.h"
#include "import/import.h"
#include "camera_manager.h"
#include "texture_manager.h"
#include "material_manager.h"
#include "mesh_manager.h"
#include "instance_manager.h"
#include "light_manager.h"
#include "aabb.h"
#include "framework/vars.h"

#include "log/log.h"

namespace core
{

/****************************************************************************/

bool loadScenefiles(const std::string& scenefiles)
{
    const float scene_scale = static_cast<float>(vars.scene_scale);
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
            inst->move(node->position * scene_scale);
            inst->setScale(node->scale * scene_scale);
            inst->setOrientation(node->rotation);
            scene_bbox.expandBy(inst->getBoundingBox());
        }

        for (unsigned int i = 0; i < scene->num_cameras; i++) {
            const auto* cam = scene->cameras[i];
            const auto pos = cam->position * scene_scale;
            res::cameras->createPerspectiveCam(cam->name, pos,
                    pos + cam->direction,
                    2.0 * glm::atan(glm::tan(glm::radians(cam->hfov) / 2.0) / cam->aspect_ratio),
                    cam->aspect_ratio, vars.cam_nearplane, vars.cam_farplane);
        }

        const auto sceneDiff = scene_bbox.pmax - scene_bbox.pmin;
        const auto maxDist = std::sqrt(glm::dot(sceneDiff, sceneDiff));

        for (unsigned int i = 0; i < scene->num_lights; ++i) {
            Light* newLight = nullptr;
            const auto* light = scene->lights[i];
            // name starts with "scl_" -> shadowcasting
            const bool isShadowcasting = (std::strncmp(light->name, "scl_", 4) == 0);
            switch (light->type) {
            case import::LightType::SPOT:
            {
                auto* l = core::res::lights->createSpotlight(isShadowcasting);
                l->setDirection(light->direction);
                l->setAngleInnerCone(light->angle_inner_cone);
                l->setAngleOuterCone(light->angle_outer_cone);
                newLight = l;
                break;
            }
            case import::LightType::DIRECTIONAL:
            {
                auto* l = core::res::lights->createDirectionalLight(isShadowcasting);
                l->setDirection(light->direction);
                l->setSize(1000.f * scene_scale); // TODO
                l->setRotation(.0f); // TODO
                newLight = l;
                break;
            }
            case import::LightType::POINT:
            {
                auto* l = core::res::lights->createPointLight(isShadowcasting);
                newLight = l;
                break;
            }
            default:
                break;
            }
            if (newLight != nullptr) {
                newLight->setPosition(light->position * scene_scale);
                newLight->setIntensity(light->color);
                newLight->setMaxDistance(light->max_distance);
                newLight->setMaxDistance(maxDist);
                newLight->setLinearAttenuation(light->linear_attenuation / scene_scale);
                newLight->setConstantAttenuation(light->constant_attenuation);
                newLight->setQuadraticAttenuation(light->quadratic_attenuation / scene_scale);
            } else {
                LOG_WARNING("Unsupported light type for light'",
                        light->name, '\'');
            }
        }
    }

    if (res::cameras->getDefaultCam() == nullptr) {
        auto* cam = res::cameras->createPerspectiveCam("default_cam", glm::dvec3(0.0),
                glm::dvec3(0.0, 0.0, 1.0), glm::radians(45.0), 4.0 / 3.0,
                vars.cam_nearplane, vars.cam_farplane);
        res::cameras->makeDefault(cam);
    }

    return result;
}

/****************************************************************************/

} // namespace core

