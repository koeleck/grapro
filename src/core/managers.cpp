#include "managers.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "camera_manager.h"

namespace core
{

/***************************************************************************/

namespace res
{
ShaderManager* shaders;
CameraManager* cameras;
TextureManager* textures;
} // namespace res

/***************************************************************************/

void initializeManagers()
{
    res::shaders = new ShaderManager();
    res::cameras = new CameraManager();
    res::textures = new TextureManager();
}

/***************************************************************************/

void terminateManagers()
{
    delete res::shaders;
    delete res::cameras;
    delete res::textures;
}

/***************************************************************************/

} // namespace core

