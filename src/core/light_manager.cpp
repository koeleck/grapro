#include "light_manager.h"
#include "framework/vars.h"
#include "log/log.h"
#include "shader_interface.h"

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

    assert(vars.max_num_2d_shadowmaps <= 32 && vars.max_num_cube_shadowmaps <= 32);

    const GLenum internalformat = gl::stringToEnum(vars.shadowmap_internalformat);
    const float border[] = {.0f, .0f, .0f, .0f};

    // 2d shadowmaps
    glBindFramebuffer(GL_FRAMEBUFFER, m_2d_fbo);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowmaps);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat,
            vars.shadowmap_res, vars.shadowmap_res,
            vars.max_num_2d_shadowmaps);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    glDrawBuffer(GL_NONE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            m_shadowmaps, 0);

    // cube shadowmaps
    glBindFramebuffer(GL_FRAMEBUFFER, m_cube_fbo);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_shadowcubemaps);
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY,  1, internalformat,
            vars.shadowcubemap_res, vars.shadowcubemap_res,
            6 * vars.max_num_cube_shadowmaps);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    glDrawBuffer(GL_NONE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            m_shadowcubemaps, 0);

    // done with framebuffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);


    // glBindBufferRange requires offset to be aligned, so compute this offset here:
    GLint alignment;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
    // round up
    const auto size_2d = vars.max_num_2d_shadowmaps * static_cast<int>(sizeof(GLint));
    m_buffer_offset = alignment * ((size_2d + alignment - 1) / alignment);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_light_id_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
            m_buffer_offset + (vars.max_num_cube_shadowmaps * static_cast<int>(sizeof(GLint))),
            nullptr, GL_DYNAMIC_STORAGE_BIT);
    const GLint negone = -1;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &negone);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


/****************************************************************************/

LightManager::~LightManager() = default;

/****************************************************************************/

SpotLight* LightManager::createSpotlight(const bool isShadowcasting)
{
    const auto offset = m_light_buffer.alloc();
    auto* const data = m_light_buffer.offsetToPointer(offset);
    const auto index = m_light_buffer.offsetToIndex(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowmaps == vars.max_num_2d_shadowmaps) {
            LOG_ERROR("Maximum number of 2d shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = m_num_shadowmaps;
            glNamedBufferSubDataEXT(m_light_id_buffer,
                    m_num_shadowmaps * static_cast<int>(sizeof(GLint)), sizeof(GLint),
                    &index);
            m_num_shadowmaps++;
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
    const auto index = m_light_buffer.offsetToIndex(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowmaps == vars.max_num_2d_shadowmaps) {
            LOG_ERROR("Maximum number of 2d shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = m_num_shadowmaps;
            glNamedBufferSubDataEXT(m_light_id_buffer,
                    m_num_shadowmaps * static_cast<int>(sizeof(GLint)), sizeof(GLint),
                    &index);
            m_num_shadowmaps++;
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
    const auto index = m_light_buffer.offsetToIndex(offset);

    int depthTex = -1;
    if (isShadowcasting) {
        if (m_num_shadowcubemaps == vars.max_num_cube_shadowmaps) {
            LOG_ERROR("Maximum number of cube shadowmaps reached! "
                    "Created light will not cast shadows.");
        } else {
            depthTex = m_num_shadowcubemaps;
            glNamedBufferSubDataEXT(m_light_id_buffer,
                    m_buffer_offset + (m_num_shadowcubemaps * static_cast<int>(sizeof(GLint))),
                    sizeof(GLint),
                    &index);
            m_num_shadowcubemaps++;
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

void LightManager::setupForShadowMapRendering()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_2d_fbo);
    glViewport(0, 0, vars.shadowmap_res, vars.shadowmap_res);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::LIGHT_IDS,
            m_light_id_buffer);
}

/****************************************************************************/

void LightManager::setupForShadowCubeMapRendering()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_cube_fbo);
    glViewport(0, 0, vars.shadowcubemap_res, vars.shadowcubemap_res);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindings::LIGHT_IDS,
            m_light_id_buffer, m_buffer_offset,
            vars.max_num_cube_shadowmaps * static_cast<int>(sizeof(GLint)));
}

/****************************************************************************/

GLuint LightManager::getShadowMapTexture() const
{
    return m_shadowmaps.get();
}

/****************************************************************************/

GLuint LightManager::getShadowCubeMapTexture() const
{
    return m_shadowcubemaps.get();
}

/****************************************************************************/

} // namespace core

