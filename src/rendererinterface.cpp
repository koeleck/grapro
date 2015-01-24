#include "rendererinterface.h"

#include <algorithm>

#include "core/mesh_manager.h"
#include "core/shader_manager.h"
#include "core/camera_manager.h"
#include "core/instance_manager.h"
#include "core/material_manager.h"
#include "core/texture.h"
#include "core/timer_array.h"

#include "log/log.h"

#include "framework/vars.h"

RendererInterface::RendererInterface(core::TimerArray& timer_array, unsigned int treeLevels)
  : m_numVoxelFrag{0u},
    m_rebuildTree{true},
    m_treeLevels{treeLevels},
  	m_timers(timer_array), // bug in gcc 4.8.2
  	m_voxelize_timer{m_timers.addGPUTimer("Voxelize")},
  	m_tree_timer{m_timers.addGPUTimer("Octree")},
    m_mipmap_timer{m_timers.addGPUTimer("Mipmap")}
{

	initBBoxes();
	initVertexPulling();
	initVoxelization();
	initVoxelBBoxes();
    initVoxelColors();
    initGBuffer();
    initConeTracingPass();
    initBrickTexture();

}

/****************************************************************************/

RendererInterface::~RendererInterface() = default;

/****************************************************************************/

void RendererInterface::setGeometry(std::vector<const core::Instance*> geometry)
{
    m_geometry = std::move(geometry);

    std::sort(m_geometry.begin(), m_geometry.end(),
            [] (const core::Instance* g0, const core::Instance* g1) -> bool
            {
                return g0->getMesh()->components() < g1->getMesh()->components();
            });

    m_scene_bbox = core::AABB();
    m_drawlist.clear();
    m_drawlist.reserve(m_geometry.size());
    for (const auto* g : m_geometry) {
        const auto* mesh = g->getMesh();
        const auto prog = m_programs[mesh->components()()];
        GLuint vao {core::res::meshes->getVAO(mesh)};
        m_drawlist.emplace_back(g, prog, vao, mesh->mode(),
                mesh->count(), mesh->type(),
                mesh->indices(), mesh->basevertex());
        m_scene_bbox.expandBy(g->getBoundingBox());
    }

    // make every side of the bounding box equally long
    const auto maxExtend = m_scene_bbox.maxExtend();
    const auto dist = (m_scene_bbox.pmax[maxExtend] - m_scene_bbox.pmin[maxExtend]) * .5f;
    const auto center = m_scene_bbox.center();
    for (int i{}; i < 3; ++i) {
        m_scene_bbox.pmin[i] = center[i] - dist;
        m_scene_bbox.pmax[i] = center[i] + dist;
    }
    // make bounding box 5% bigger
    m_scene_bbox.pmin -= .05f * dist;
    m_scene_bbox.pmax += .05f * dist;

    LOG_INFO("Scene bounding box: [",
            m_scene_bbox.pmin.x, ", ",
            m_scene_bbox.pmin.y, ", ",
            m_scene_bbox.pmin.z, "] -> [",
            m_scene_bbox.pmax.x, ", ",
            m_scene_bbox.pmax.y, ", ",
            m_scene_bbox.pmax.z, "]");

    m_voxelize_cam->setLeft(m_scene_bbox.pmin.x);
    m_voxelize_cam->setRight(m_scene_bbox.pmax.x);
    m_voxelize_cam->setBottom(m_scene_bbox.pmin.y);
    m_voxelize_cam->setTop(m_scene_bbox.pmax.y);
    m_voxelize_cam->setZNear(-m_scene_bbox.pmin.z);
    m_voxelize_cam->setZFar(-m_scene_bbox.pmax.z);
    // set view matrix to identity
    m_voxelize_cam->setPosition(glm::dvec3(0.));
    m_voxelize_cam->setOrientation(glm::dquat());
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));

}

/****************************************************************************/

void RendererInterface::initBBoxes()
{
    // indices
    GLubyte indices[] = {0, 1,
                         0, 2,
                         0, 4,
                         1, 3,
                         1, 5,
                         2, 3,
                         2, 6,
                         3, 7,
                         4, 5,
                         4, 6,
                         5, 7,
                         6, 7};

    glBindVertexArray(m_bbox_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bbox_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    core::res::shaders->registerShader("bbox_vert", "basic/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("bbox_frag", "basic/bbox.frag", GL_FRAGMENT_SHADER);
    m_bbox_prog = core::res::shaders->registerProgram("bbox_prog", {"bbox_vert", "bbox_frag"});
}

/****************************************************************************/

void RendererInterface::initVertexPulling()
{
	core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("vertexpulling_frag", "basic/vertexpulling.frag",
            GL_FRAGMENT_SHADER);
    m_vertexpulling_prog = core::res::shaders->registerProgram("vertexpulling_prog",
            {"vertexpulling_vert", "vertexpulling_frag"});

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);
}

/****************************************************************************/

void RendererInterface::initVoxelization()
{
	core::res::shaders->registerShader("voxelGeom", "tree/voxelize.geom", GL_GEOMETRY_SHADER);
    core::res::shaders->registerShader("voxelFrag", "tree/voxelize.frag", GL_FRAGMENT_SHADER);
    m_voxel_prog = core::res::shaders->registerProgram("voxel_prog",
            {"vertexpulling_vert", "voxelGeom", "voxelFrag"});

    m_voxelize_cam = core::res::cameras->createOrthogonalCam("voxelization_cam",
            glm::dvec3(0.0), glm::dvec3(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));

    resizeFBO();
}

/****************************************************************************/

void RendererInterface::initVoxelBBoxes()
{
	core::res::shaders->registerShader("octreeDebugBBox_vert", "tree/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("octreeDebugBBox_frag", "tree/bbox.frag", GL_FRAGMENT_SHADER);
    m_voxel_bbox_prog = core::res::shaders->registerProgram("octreeDebugBBox_prog",
            {"octreeDebugBBox_vert", "octreeDebugBBox_frag"});
}

/****************************************************************************/

void RendererInterface::initVoxelColors()
{
    core::res::shaders->registerShader("colorboxes_vert", "tree/colorboxes.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("colorboxes_geom", "tree/colorboxes.geom", GL_GEOMETRY_SHADER);
    core::res::shaders->registerShader("colorboxes_frag", "tree/colorboxes.frag", GL_FRAGMENT_SHADER);
    m_colorboxes_prog = core::res::shaders->registerProgram("colorboxes_prog",
            {"colorboxes_vert", "colorboxes_geom", "colorboxes_frag"});
}



void RendererInterface::initConeTracingPass()
{
    core::res::shaders->registerShader("ssq_ao_vert", "conetracing/ssq_ao.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("conetracing_frag", "conetracing/conetracing.frag", GL_FRAGMENT_SHADER);
    m_coneTracing_prog = core::res::shaders->registerProgram("coneTracing_prog", {"ssq_ao_vert", "conetracing_frag"});
}

/****************************************************************************/

void RendererInterface::initGBuffer()
{
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("gbuffer_frag", "conetracing/gbuffer.frag", GL_FRAGMENT_SHADER);
    m_gbuffer_prog = core::res::shaders->registerProgram("gbuffer_prog", {"vertexpulling_vert", "gbuffer_frag"});

    // textures
    glBindTexture(GL_TEXTURE_2D, m_tex_position);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, vars.screen_width, vars.screen_height);

    glBindTexture(GL_TEXTURE_2D, m_tex_normal);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, vars.screen_width, vars.screen_height);

    glBindTexture(GL_TEXTURE_2D, m_tex_color);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, vars.screen_width, vars.screen_height);

    glBindTexture(GL_TEXTURE_2D, m_tex_depth);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, vars.screen_width, vars.screen_height);

    // FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer_FBO);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_tex_depth, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_position, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_tex_normal, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, m_tex_color, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty gbuffer framebuffer is not complete (WTF?!?)");
    }
    GLenum DrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, DrawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // screen space quad
    glBindVertexArray(m_vao_ssq);
    float ssq[] = { -1.f, -1.f, 0.f,
                    1.f, -1.f, 0.f,
                    1.f, 1.f, 0.f,
                    1.f, 1.f, 0.f,
                    -1.f, 1.f, 0.f,
                    -1.f, -1.f, 0.f };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_ssq);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ssq), ssq, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
}

/****************************************************************************/

void RendererInterface::initBrickTexture()
{
    GLint max_3d_tex_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_tex_size);
    //int remaining = static_cast<int>(vars.max_voxel_nodes);
    int remaining = static_cast<int>(2097152);
    int brick_num_x = std::min(remaining, max_3d_tex_size / 3);
    remaining = (remaining + brick_num_x - 1) / brick_num_x;
    int brick_num_y = std::min(remaining, max_3d_tex_size / 3);
    remaining = (remaining + brick_num_y - 1) / brick_num_y;
    int brick_num_z = std::min(remaining, max_3d_tex_size / 3);
    //if ((brick_num_x * brick_num_y * brick_num_z) < static_cast<int>(vars.max_voxel_nodes)) {
    if ((brick_num_x * brick_num_y * brick_num_z) < static_cast<int>(2097152)) {
        LOG_ERROR("brick texture can't store all nodes!");
        abort();
    }
    glBindTexture(GL_TEXTURE_3D, m_brick_texture);
    //constexpr int BRICK_TEX_ELEMENT_SIZE = 2 * 4;
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, brick_num_x, brick_num_y, brick_num_z);
    m_brick_texture_size.x = brick_num_x * 3;
    m_brick_texture_size.y = brick_num_y * 3;
    m_brick_texture_size.z = brick_num_z * 3;
}

void RendererInterface::recreateBuffer(gl::Buffer & buf, const size_t size) const
{
    gl::Buffer tmp;
    buf.swap(tmp);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_READ_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/****************************************************************************/

void RendererInterface::renderBoundingBoxes() const
{
    if (m_geometry.empty())
        return;

    core::res::instances->bind();

    glUseProgram(m_bbox_prog);
    glBindVertexArray(m_bbox_vao);

    glDisable(GL_DEPTH_TEST);

    for (const auto* g : m_geometry) {
        glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE,
                nullptr, 1, g->getMesh()->basevertex(), g->getIndex());
    }
}

/****************************************************************************/

void RendererInterface::renderVoxelBoundingBoxes() const
{
    if (m_voxel_bboxes.empty())
        return;

    glUseProgram(m_voxel_bbox_prog);
    glBindVertexArray(m_bbox_vao);

    glEnable(GL_DEPTH_TEST);

    for (auto i = 0u; i < m_voxel_bboxes.size(); ++i) {
        const auto & bbox = m_voxel_bboxes[i];
        float data[6] = {bbox.pmin.x, bbox.pmin.y, bbox.pmin.z,
                         bbox.pmax.x, bbox.pmax.y, bbox.pmax.z};
        glUniform3fv(0, 2, data);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr);
    }
}

/****************************************************************************/

void RendererInterface::createVoxelBBoxes(const unsigned int num)
{
    std::vector<OctreeNodeStruct> nodes(num);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num * sizeof(OctreeNodeStruct), nodes.data());

    m_voxel_bboxes.clear();
    m_voxel_bboxes.reserve(num);
    std::pair<GLuint, core::AABB> stack[128];
    stack[0] = std::make_pair(0u, m_scene_bbox);
    std::size_t top {1};
    do {
        top--;
        const auto idx = stack[top].first;
        const auto bbox = stack[top].second;
        const auto childidx = nodes[idx].id;
        if (childidx == 0x80000000) {
            m_voxel_bboxes.emplace_back(bbox);
        } else if ((childidx & 0x80000000) != 0) {
            const auto baseidx = uint(childidx & 0x7FFFFFFFu);
            const auto c = bbox.center();
            for (int i{}; i < 8; ++i) {
                int x {(i>>0) & 0x01};
                int y {(i>>1) & 0x01};
                int z {(i>>2) & 0x01};

                core::AABB newBBox;
                newBBox.pmin.x = {(x == 0) ? bbox.pmin.x : c.x};
                newBBox.pmin.y = {(y == 0) ? bbox.pmin.y : c.y};
                newBBox.pmin.z = {(z == 0) ? bbox.pmin.z : c.z};

                newBBox.pmax.x = {(x == 0) ? c.x : bbox.pmax.x};
                newBBox.pmax.y = {(y == 0) ? c.y : bbox.pmax.y};
                newBBox.pmax.z = {(z == 0) ? c.z : bbox.pmax.z};

                stack[top++] = std::make_pair(baseidx + i, newBBox);
            }
        }
    } while (top != 0);
}

/****************************************************************************/

void RendererInterface::renderGeometry(const GLuint prog) const
{
    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    const auto* cam = core::res::cameras->getDefaultCam();

    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    glUseProgram(prog);
    glBindVertexArray(m_vertexpulling_vao);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        // bind textures
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasDiffuseTexture()) {
            const unsigned int unit {core::bindings::DIFFUSE_TEX_UNIT};
            GLuint tex {*mat->getDiffuseTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasSpecularTexture()) {
            const unsigned int unit {core::bindings::SPECULAR_TEX_UNIT};
            GLuint tex {*mat->getSpecularTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasGlossyTexture()) {
            const unsigned int unit {core::bindings::GLOSSY_TEX_UNIT};
            GLuint tex {*mat->getGlossyTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasNormalTexture()) {
            const unsigned int unit {core::bindings::NORMAL_TEX_UNIT};
            GLuint tex {*mat->getNormalTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasEmissiveTexture()) {
            const unsigned int unit {core::bindings::EMISSIVE_TEX_UNIT};
            GLuint tex {*mat->getEmissiveTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAlphaTexture()) {
            const unsigned int unit {core::bindings::ALPHA_TEX_UNIT};
            GLuint tex {*mat->getAlphaTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAmbientTexture()) {
            const unsigned int unit {core::bindings::AMBIENT_TEX_UNIT};
            GLuint tex {*mat->getAmbientTexture()};
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }

        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, 0, cmd.instance->getIndex());
    }
}

/****************************************************************************/

void RendererInterface::resizeFBO() const
{
    const auto num_voxels = static_cast<int>(std::pow(2, m_treeLevels - 1));
    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, num_voxels);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, num_voxels);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty framebuffer is not complete (WTF?!?)");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/****************************************************************************/

unsigned int RendererInterface::calculateMaxNodes() const
{
    auto maxNodes = 1u;
    auto tmp = 1u;
    for (auto i = 0u; i < m_treeLevels; ++i) {
        tmp *= 8;
        maxNodes += tmp;
    }
    return maxNodes;
}

/****************************************************************************/

void RendererInterface::renderVoxelColors() const
{

    if (m_voxel_bboxes.empty())
        return;

    glUseProgram(m_colorboxes_prog);
    glBindVertexArray(m_colorboxes_vao);

    // uniforms
    const auto dim = static_cast<int>(std::pow(2.0, m_treeLevels - 1));
    auto loc = glGetUniformLocation(m_colorboxes_prog, "u_bboxMin");
    glUniform3f(loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_bboxMax");
    glUniform3f(loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_voxelDim");
    glUniform1ui(loc, dim);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_maxLevel");
    glUniform1ui(loc, m_treeLevels);

    for (auto i = 0u; i < m_voxel_bboxes.size(); ++i) {
        const auto & bbox = m_voxel_bboxes[i];
        float data[3] = {bbox.center().x, bbox.center().y, bbox.center().z};
        glUniform3fv(0, 1, data);
        glDrawArrays(GL_POINTS, 0, 1);
    }

}

/****************************************************************************/

void RendererInterface::renderToGBuffer() const
{

    // bind gbuffer texture
    glBindTexture(GL_TEXTURE_2D, m_tex_position.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_normal.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_color.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_depth.get());

    // FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer_FBO);
    glViewport(0, 0, vars.screen_width, vars.screen_height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderGeometry(m_gbuffer_prog);

    // unbind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

/****************************************************************************/

void RendererInterface::renderIndirectDiffuseLighting() const
{

    renderToGBuffer();

    /*
     *  Render screen space quad
     */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_indirectDiffuse_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_position);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_tex_normal);

    auto loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_pos");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_normal");
    glUniform1i(loc, 1);

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_voxelDim");
    glUniform1ui(loc, voxelDim);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_bboxMin");
    glUniform3f(loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_bboxMax");
    glUniform3f(loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_screenwidth");
    glUniform1ui(loc, vars.screen_width);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_screenheight");
    glUniform1ui(loc, vars.screen_height);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_treeLevels");
    glUniform1ui(loc, m_treeLevels);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_coneGridSize");
    glUniform1ui(loc, m_options.diffuseConeGridSize);
    loc = glGetUniformLocation(m_indirectDiffuse_prog, "u_numSteps");
    glUniform1ui(loc, m_options.diffuseConeSteps);

    glBindVertexArray(m_vao_ssq);
    glDrawArrays(GL_TRIANGLES, 0, 6);

}

/****************************************************************************/

void RendererInterface::renderIndirectSpecularLighting() const
{

    renderToGBuffer();

    /*
     *  Render screen space quad
     */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_indirectSpecular_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_position);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_tex_normal);

    auto loc = glGetUniformLocation(m_indirectSpecular_prog, "u_pos");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_normal");
    glUniform1i(loc, 1);

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_voxelDim");
    glUniform1ui(loc, voxelDim);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_bboxMin");
    glUniform3f(loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_bboxMax");
    glUniform3f(loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_screenwidth");
    glUniform1ui(loc, vars.screen_width);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_screenheight");
    glUniform1ui(loc, vars.screen_height);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_treeLevels");
    glUniform1ui(loc, m_treeLevels);
    loc = glGetUniformLocation(m_indirectSpecular_prog, "u_numSteps");
    glUniform1ui(loc, m_options.specularConeSteps);

    glBindVertexArray(m_vao_ssq);
    glDrawArrays(GL_TRIANGLES, 0, 6);

}

/****************************************************************************/

void RendererInterface::coneTracing() const
{
    renderToGBuffer();

    /*
     *  Render screen space quad
     */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_coneTracing_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_position);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_tex_normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_tex_color);

    auto loc = glGetUniformLocation(m_coneTracing_prog, "u_pos");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_normal");
    glUniform1i(loc, 1);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_color");
    glUniform1i(loc, 2);

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));
    loc = glGetUniformLocation(m_coneTracing_prog, "u_voxelDim");
    glUniform1ui(loc, voxelDim);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_bboxMin");
    glUniform3f(loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_bboxMax");
    glUniform3f(loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_screenwidth");
    glUniform1ui(loc, vars.screen_width);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_screenheight");
    glUniform1ui(loc, vars.screen_height);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_treeLevels");
    glUniform1ui(loc, m_treeLevels);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_diffuseConeGridSize");
    glUniform1ui(loc, m_options.diffuseConeGridSize);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_diffuseConeSteps");
    glUniform1ui(loc, m_options.diffuseConeSteps);
    loc = glGetUniformLocation(m_coneTracing_prog, "u_specularConeSteps");
    glUniform1ui(loc, m_options.specularConeSteps);

    glBindVertexArray(m_vao_ssq);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

/****************************************************************************/
