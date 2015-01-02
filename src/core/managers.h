#ifndef CORE_MANAGERS_H
#define CORE_MANAGERS_H

namespace core
{

class ShaderManager;
class CameraManager;
class TextureManager;
class MaterialManager;

namespace res
{
extern ShaderManager* shaders;
extern CameraManager* cameras;
extern TextureManager* textures;
extern MaterialManager* materials;
} // namespace res


void initializeManagers();
void terminateManagers();

} // namespace core

#endif // CORE_MANAGERS_H

