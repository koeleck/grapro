#include "managers.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "camera_manager.h"
#include "material_manager.h"
#include "instance_manager.h"

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
} // namespace res

/***************************************************************************/

void initializeManagers()
{
    res::shaders = new ShaderManager();
    res::cameras = new CameraManager();
    res::textures = new TextureManager();
    res::materials = new MaterialManager();
    res::instances = new InstanceManager();
}

/***************************************************************************/

void terminateManagers()
{
    delete res::shaders;
    delete res::cameras;
    delete res::textures;
    delete res::materials;
    delete res::instances;
}

/***************************************************************************/

} // namespace core

