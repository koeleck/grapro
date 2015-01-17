#include "managers.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "camera_manager.h"
#include "material_manager.h"
#include "mesh_manager.h"
#include "instance_manager.h"
#include "light_manager.h"

namespace core
{

/***************************************************************************/

namespace res
{
ShaderManager* shaders;
CameraManager* cameras;
TextureManager* textures;
MaterialManager* materials;
InstanceManager* instances;
MeshManager* meshes;
LightManager* lights;
} // namespace res

/***************************************************************************/

void initializeManagers()
{
    res::shaders = new ShaderManager();
    res::cameras = new CameraManager();
    res::textures = new TextureManager();
    res::materials = new MaterialManager();
    res::instances = new InstanceManager();
    res::meshes = new MeshManager();
    res::lights = new LightManager();
}

/***************************************************************************/

void terminateManagers()
{
    delete res::shaders;
    delete res::cameras;
    delete res::textures;
    delete res::materials;
    delete res::instances;
    delete res::meshes;
    delete res::lights;
}

/***************************************************************************/

bool updateManagers()
{
    bool result = false;

    result |= res::cameras->update();
    result |= res::instances->update();

    return result;
}

/***************************************************************************/

} // namespace core

