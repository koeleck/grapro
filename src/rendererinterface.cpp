#include "rendererinterface.h"

#include <algorithm>

#include "core/mesh.h"
#include "core/mesh_manager.h"
#include "core/shader_manager.h"
#include "core/camera.h"
#include "core/camera_manager.h"
#include "core/instance.h"
#include "core/instance_manager.h"
#include "core/material_manager.h"
#include "core/texture.h"
#include "core/timer_array.h"

#include "log/log.h"

#include "framework/vars.h"

RendererInterface::RendererInterface(core::TimerArray& timer_array)
  : m_numVoxelFrag{0u},
  	m_timers{timer_array},
  	m_voxelize_timer{m_timers.addGPUTimer("Voxelize")},
  	m_tree_timer{m_timers.addGPUTimer("Octree")},
    m_rebuildTree{true}
{

	initBBoxes();
	initVertexPulling();
	initVoxelization();
	initVoxelBBoxes();

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
        GLuint vao = core::res::meshes->getVAO(mesh);
        m_drawlist.emplace_back(g, prog, vao, mesh->mode(),
                mesh->count(), mesh->type(),
                mesh->indices(), mesh->basevertex());
        m_scene_bbox.expandBy(g->getBoundingBox());
    }

    // make every side of the bounding box equally long
    const auto maxExtend = m_scene_bbox.maxExtend();
    const auto dist = (m_scene_bbox.pmax[maxExtend] - m_scene_bbox.pmin[maxExtend]) * .5f;
    const auto center = m_scene_bbox.center();
    for (int i = 0; i < 3; ++i) {
        m_scene_bbox.pmin[i] = center[i] - dist;
        m_scene_bbox.pmax[i] = center[i] + dist;
    }

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

    // FBO
    int num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));
    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, num_voxels);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, num_voxels);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty framebuffer is not complete (WTF?!?)");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void RendererInterface::renderBoundingBoxes()
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

void RendererInterface::renderVoxelBoundingBoxes()
{
    if (m_voxel_bboxes.empty())
        return;

    glUseProgram(m_voxel_bbox_prog);
    glBindVertexArray(m_bbox_vao);

    glEnable(GL_DEPTH_TEST);

    for (const auto& bbox : m_voxel_bboxes) {
        float data[6] = {bbox.pmin.x, bbox.pmin.y, bbox.pmin.z,
                         bbox.pmax.x, bbox.pmax.y, bbox.pmax.z};
        glUniform3fv(0, 2, data);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr);
    }

}

/****************************************************************************/

void RendererInterface::createVoxelBBoxes(const unsigned int num)
{
    std::vector<GLuint> nodes(num);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num * sizeof(GLuint), nodes.data());

    m_voxel_bboxes.clear();
    m_voxel_bboxes.reserve(num);
    std::pair<GLuint, core::AABB> stack[128];
    stack[0] = std::make_pair(0u, m_scene_bbox);
    std::size_t top = 1;
    do {
        top--;
        const auto idx = stack[top].first;
        const auto bbox = stack[top].second;

        const auto childidx = nodes[idx];
        if (childidx == 0x80000000) {
            m_voxel_bboxes.emplace_back(bbox);
        } else if ((childidx & 0x80000000) != 0) {
            const auto baseidx = uint(childidx & 0x7FFFFFFFu);
            const auto c = bbox.center();
            for (unsigned int i = 0; i < 8; ++i) {
                int x = (i>>0) & 0x01;
                int y = (i>>1) & 0x01;
                int z = (i>>2) & 0x01;

                core::AABB newBBox;
                newBBox.pmin.x = (x == 0) ? bbox.pmin.x : c.x;
                newBBox.pmin.y = (y == 0) ? bbox.pmin.y : c.y;
                newBBox.pmin.z = (z == 0) ? bbox.pmin.z : c.z;

                newBBox.pmax.x = (x == 0) ? c.x : bbox.pmax.x;
                newBBox.pmax.y = (y == 0) ? c.y : bbox.pmax.y;
                newBBox.pmax.z = (z == 0) ? c.z : bbox.pmax.z;

                stack[top++] = std::make_pair(baseidx + i, newBBox);
            }
        }
    } while (top != 0);
}

/****************************************************************************/

void RendererInterface::renderGeometry(const GLuint prog)
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
            unsigned int unit = core::bindings::DIFFUSE_TEX_UNIT;
            GLuint tex = *mat->getDiffuseTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasSpecularTexture()) {
            unsigned int unit = core::bindings::SPECULAR_TEX_UNIT;
            GLuint tex = *mat->getSpecularTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasGlossyTexture()) {
            unsigned int unit = core::bindings::GLOSSY_TEX_UNIT;
            GLuint tex = *mat->getGlossyTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasNormalTexture()) {
            unsigned int unit = core::bindings::NORMAL_TEX_UNIT;
            GLuint tex = *mat->getNormalTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasEmissiveTexture()) {
            unsigned int unit = core::bindings::EMISSIVE_TEX_UNIT;
            GLuint tex = *mat->getEmissiveTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAlphaTexture()) {
            unsigned int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = *mat->getAlphaTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAmbientTexture()) {
            unsigned int unit = core::bindings::AMBIENT_TEX_UNIT;
            GLuint tex = *mat->getAmbientTexture();
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
