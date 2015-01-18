#ifndef CORE_LIGHT_MANAGER_H
#define CORE_LIGHT_MANAGER_H

#include <vector>
#include <memory>

#include "gl/gl_objects.h"
#include "buffer_storage_pool.h"
#include "shader_interface.h"
#include "light.h"

namespace core
{

class LightManager
{
public:
    using LightList = std::vector<std::unique_ptr<Light>>;

    LightManager();
    ~LightManager();

    SpotLight* createSpotlight(bool isShadowcasting);
    DirectionalLight* createDirectionalLight(bool isShadowcasting);
    PointLight* createPointLight(bool isShadowcasting);

    const LightList& getLights() const;
    void bind() const;

    void setupForShadowMapRendering();
    void setupForShadowCubeMapRendering();

    GLuint getShadowMapTexture() const;
    GLuint getShadowCubeMapTexture() const;

private:
    using LightPool = BufferStoragePool<shader::LightStruct>;

    LightList       m_lights;
    gl::Texture     m_shadowmaps;
    gl::Texture     m_shadowcubemaps;
    gl::Framebuffer m_2d_fbo;
    gl::Framebuffer m_cube_fbo;
    gl::Buffer      m_light_id_buffer;
    LightPool       m_light_buffer;
    int             m_num_shadowmaps;
    int             m_num_shadowcubemaps;

};

} // namespace core

#endif // CORE_LIGHT_MANAGER_H

