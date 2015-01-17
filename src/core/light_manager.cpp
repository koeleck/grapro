#include "light_manager.h"
#include "framework/vars.h"
#include "log/log.h"

namespace core
{

/****************************************************************************/

LightManager::LightManager()
  : m_light_buffer(GL_SHADER_STORAGE_BUFFER,
          static_cast<std::size_t>(vars.max_num_lights)),
    m_num_shadowmaps{0},
    m_num_shadowcubemaps{0}
{
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    const GLenum internalformat = gl::stringToEnum(vars.shadowmap_internalformat);
    const float border[] = {.0f, .0f, .0f, .0f};

    // 2d shadowmaps
    glBindFramebuffer(GL_FRAMEBUFFER, m_2d_fbo);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowmaps);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat,
            vars.shadowmap_res, vars.shadowmap_res,
            vars.max_num_2d_shadowmaps);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            m_shadowmaps, 0);

    // cube shadowmaps
    glBindFramebuffer(GL_FRAMEBUFFER, m_cube_fbo);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_shadowcubemaps);
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY,  1, internalformat,
            vars.shadowcubemap_res, vars.shadowcubemap_res,
            6 * vars.max_num_cube_shadowmaps);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            m_shadowcubemaps, 0);

    // done
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}


/****************************************************************************/

LightManager::~LightManager() = default;

/****************************************************************************/

SpotLight* LightManager::createSpotlight(const bool isShadowcasting)
{
    const auto offset = m_light_buffer.alloc();
    auto* const data = m_light_buffer.offsetToPointer(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowmaps == vars.max_num_2d_shadowmaps) {
            LOG_ERROR("Maximum number of 2d shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = m_num_shadowmaps++;
        }
    }

    auto ptr = std::unique_ptr<Light>(new SpotLight(data, depthTex));
    m_lights.emplace_back(std::move(ptr));

    return static_cast<SpotLight*>(m_lights.back().get());
}

/****************************************************************************/

DirectionalLight* LightManager::createDirectionalLight(const bool isShadowcasting)
{
    const auto offset = m_light_buffer.alloc();
    auto* const data = m_light_buffer.offsetToPointer(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowmaps == vars.max_num_2d_shadowmaps) {
            LOG_ERROR("Maximum number of 2d shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = m_num_shadowmaps++;
        }
    }

    auto ptr = std::unique_ptr<Light>(new DirectionalLight(data, depthTex));
    m_lights.emplace_back(std::move(ptr));

    return static_cast<DirectionalLight*>(m_lights.back().get());
}

/****************************************************************************/

PointLight* LightManager::createPointLight(const bool isShadowcasting)
{
    const auto offset = m_light_buffer.alloc();
    auto* const data = m_light_buffer.offsetToPointer(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowcubemaps == vars.max_num_cube_shadowmaps) {
            LOG_ERROR("Maximum number of cube shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = 6 * m_num_shadowcubemaps++;
        }
    }

    auto ptr = std::unique_ptr<Light>(new PointLight(data, depthTex));
    m_lights.emplace_back(std::move(ptr));

    return static_cast<PointLight*>(m_lights.back().get());
}

/****************************************************************************/

const LightManager::LightList& LightManager::getLights() const
{
    return m_lights;
}

/****************************************************************************/

void LightManager::bind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::LIGHT,
            m_light_buffer.buffer());
}

/****************************************************************************/

} // namespace core

