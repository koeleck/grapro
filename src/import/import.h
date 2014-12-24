#ifndef IMPORT_IMPORT_H
#define IMPORT_IMPORT_H

#include <memory>
#include <string>

#include "camera.h"
#include "image.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "node.h"
#include "scene.h"
#include "texture.h"

namespace import
{


std::unique_ptr<Scene> importSceneFile(const std::string& filename);

} // namespace import

#endif // IMPORT_IMPORT_H

