#include "managers.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "camera_manager.h"
#include "material_manager.h"

namespace core
{

/***************************************************************************/

namespace res
{
ShaderManager* shaders;
CameraManager* cameras;
TextureManager* textures;
MaterialManager* materials;
} // namespace res

/***************************************************************************/

void initializeManagers()
{
    res::shaders = new ShaderManager();
    res::cameras = new CameraManager();
    res::textures = new TextureManager();
    res::materials = new MaterialManager();
}

/***************************************************************************/

void terminateManagers()
{
    delete res::shaders;
    delete res::cameras;
    delete res::textures;
    delete res::materials;
}

/***************************************************************************/

} // namespace core

