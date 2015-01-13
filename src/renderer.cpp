#include <cassert>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "renderer.h"
#include "core/mesh.h"
#include "core/shader_manager.h"
#include "core/mesh_manager.h"
#include "core/instance_manager.h"
#include "core/camera_manager.h"
#include "core/material_manager.h"
#include "core/shader_interface.h"
#include "core/texture.h"
#include "log/log.h"
#include "framework/vars.h"
#include "voxel.h"

/****************************************************************************/

struct Renderer::DrawCmd
{
    DrawCmd(const core::Instance* instance_, core::Program prog_,
            GLuint vao_, GLenum mode_, GLsizei count_, GLenum type_,
            GLvoid* indices_, GLint basevertex_)
      : instance{instance_},
        prog{prog_},
        vao{vao_},
        mode{mode_},
        count{count_},
        type{type_},
        indices{indices_},
        basevertex{basevertex_}
    {
    }

    const core::Instance*   instance;
    core::Program           prog;
    GLuint                  vao;
    GLenum                  mode;
    GLsizei                 count;
    GLenum                  type;
    GLvoid*                 indices;
    GLint                   basevertex;
};

/****************************************************************************/

Renderer::Renderer(core::TimerArray& timer_array)
  : m_numVoxelFrag{0u},
    m_rebuildTree{true},
    m_timers{timer_array}
{

    initBBoxStuff();

    // programmable vertex pulling
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("vertexpulling_frag", "basic/vertexpulling.frag",
            GL_FRAGMENT_SHADER);
    m_vertexpulling_prog = core::res::shaders->registerProgram("vertexpulling_prog",
            {"vertexpulling_vert", "vertexpulling_frag"});

    // voxel creation
    core::res::shaders->registerShader("voxelGeom", "tree/voxelize.geom", GL_GEOMETRY_SHADER);
    core::res::shaders->registerShader("voxelFrag", "tree/voxelize.frag", GL_FRAGMENT_SHADER);
    m_voxel_prog = core::res::shaders->registerProgram("voxel_prog",
            {"vertexpulling_vert", "voxelGeom", "voxelFrag"});

    // octree building
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag.comp", GL_COMPUTE_SHADER);
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc.comp", GL_COMPUTE_SHADER);
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

    m_voxelize_timer = m_timers.addGPUTimer("Voxelize");
    m_tree_timer = m_timers.addGPUTimer("Octree");

    // allocate voxelBuffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelStruct), nullptr, GL_MAP_READ_BIT);

    // allocate octreeBuffer
    // calculate max possible size of octree
    unsigned int totalNodes = 1;
    unsigned int tmp = 1;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {
        tmp *= 8;
        totalNodes += tmp;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, totalNodes * sizeof(OctreeNodeStruct), nullptr, GL_MAP_READ_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // debug render
    core::res::shaders->registerShader("octreeDebugBBox_vert", "tree/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("octreeDebugBBox_frag", "tree/bbox.frag", GL_FRAGMENT_SHADER);
    m_voxel_bbox_prog = core::res::shaders->registerProgram("octreeDebugBBox_prog",
            {"octreeDebugBBox_vert", "octreeDebugBBox_frag"});

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);

    m_voxelize_cam = core::res::cameras->createOrthogonalCam("voxelization_cam",
            glm::dvec3(0.0), glm::dvec3(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));

    // FBO for voxelization
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

Renderer::~Renderer() = default;

/****************************************************************************/

void Renderer::setGeometry(std::vector<const core::Instance*> geometry)
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

void Renderer::genAtomicBuffer()
{
    GLuint initVal = 0;

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initVal, GL_STATIC_DRAW);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

/****************************************************************************/

void Renderer::genVoxelBuffer()
{

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
}

/****************************************************************************/

void Renderer::genOctreeNodeBuffer()
{

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);

    // fill with zeroes
    const GLuint zero = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
}

/****************************************************************************/

void Renderer::createVoxelList(const bool debug_output)
{

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("###########################");
        LOG_INFO("# Voxel Fragment Creation #");
        LOG_INFO("###########################");
    }

    m_voxelize_timer->start();

    auto* old_cam = core::res::cameras->getDefaultCam();
    core::res::cameras->makeDefault(m_voxelize_cam);

    const auto dim = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));

    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glViewport(0, 0, dim, dim);

    glUseProgram(m_voxel_prog);

    // uniforms
    GLint loc;
    loc = glGetUniformLocation(m_voxel_prog, "uNumVoxels");
    glUniform1i(loc, dim);

    // buffer
    genAtomicBuffer();
    genVoxelBuffer();

    // render
    glBindVertexArray(m_vertexpulling_vao);
    for (const auto & cmd : m_drawlist) {

        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, 0, cmd.instance->getIndex());

    }

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    auto count = static_cast<GLuint *>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    m_numVoxelFrag = count[0];

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);

    m_voxelize_timer->stop();

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag);
        LOG_INFO("");
    }

}

/****************************************************************************/

void Renderer::buildVoxelTree(const bool debug_output)
{

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("###########################");
        LOG_INFO("#     Octree Creation     #");
        LOG_INFO("###########################");
    }

    m_tree_timer->start();

    // calculate max threads for flag shader to get all the voxel fragments
    const auto dataWidth = 1024u;
    const auto dataHeight = static_cast<unsigned int>((m_numVoxelFrag + dataWidth - 1) / dataWidth);
    const auto groupDimX = static_cast<unsigned int>(dataWidth / 8);
    const auto groupDimY = static_cast<unsigned int>((dataHeight + 7) / 8);
    const auto allocwidth = 64u;

    // octree buffer
    genOctreeNodeBuffer();

    // atomic counter (counts how many child node have to be allocated)
    genAtomicBuffer();
    const GLuint zero = 0; // for resetting the atomic counter

    // uniforms
    GLint loc_u_numVoxelFrag = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
    GLint loc_u_voxelDim = glGetUniformLocation(m_octreeNodeFlag_prog, "u_voxelDim");
    GLint loc_u_maxLevel = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");
    GLint loc_u_numNodesThisLevel = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_numNodesThisLevel");
    GLint loc_u_nodeOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_nodeOffset");
    GLint loc_u_allocOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_allocOffset");

    const unsigned int voxelDim = static_cast<unsigned int>(std::pow(2, vars.voxel_octree_levels));
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_numVoxelFrag, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_voxelDim, voxelDim);

    unsigned int nodeOffset = 0; // offset to first node of next level
    unsigned int allocOffset = 1; // offset to first free space for new nodes
    std::vector<unsigned int> maxNodesPerLevel(1, 1); // number of nodes in each tree level; root level = 1

    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        if (debug_output) {
            LOG_INFO("");
            LOG_INFO("Starting with max level ", i);
        }

        /*
         *  flag nodes
         */

        glUseProgram(m_octreeNodeFlag_prog);

        // uniforms
        glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_maxLevel, i);

        // dispatch
        if (debug_output) {
            LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
            LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
        }
        glDispatchCompute(groupDimX, groupDimY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // break to avoid allocating with ptrs to more children
        if (i == vars.voxel_octree_levels - 1) {

            nodeOffset += maxNodesPerLevel[i];
            break;

        }

        /*
         *  allocate child nodes
         */

        glUseProgram(m_octreeNodeAlloc_prog);

        // uniforms
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_numNodesThisLevel, maxNodesPerLevel[i]);
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_nodeOffset, nodeOffset);
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_allocOffset, allocOffset);

        // dispatch
        const unsigned int allocGroupDim = (maxNodesPerLevel[i] + allocwidth - 1) / allocwidth;
        if (debug_output) {
            LOG_INFO("Dispatching NodeAlloc with ", allocGroupDim, "*1*1 groups with 64*1*1 threads each");
            LOG_INFO("--> ", allocGroupDim * 64, " threads");
        }
        glDispatchCompute(allocGroupDim, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        /*
         *  get how many nodes have to be allocated at most
         */

        GLuint actualNodesAllocated;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &actualNodesAllocated);
        glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); //reset counter to zero
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        if (debug_output) { LOG_INFO(actualNodesAllocated, " nodes have been allocated"); }
        const unsigned int maxNodesToBeAllocated = actualNodesAllocated * 8;

        /*
         *  update offsets
         */

        maxNodesPerLevel.emplace_back(maxNodesToBeAllocated);   // maxNodesToBeAllocated is the number of threads
                                                                // we want to launch in the next level
        nodeOffset += maxNodesPerLevel[i];                      // add number of newly allocated child nodes to offset
        allocOffset += maxNodesToBeAllocated;

    }

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("Total Nodes created: ", nodeOffset);
        LOG_INFO("");
    }

    m_tree_timer->stop();

    /*
     *  write information to leafs
     */

    /*LOG_INFO("\nfilling leafs...");
    glUseProgram(m_octreeLeafStore_prog);

    // uniforms
    loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_numVoxelFrag");
    glUniform1ui(loc, m_numVoxelFrag);
    loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_treeLevels");
    glUniform1ui(loc, vars.voxel_octree_levels);
    loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_maxLevel");
    glUniform1ui(loc, vars.voxel_octree_levels);

    // dispatch
    LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
    LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
    glDispatchCompute(groupDimX, groupDimY, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);*/

    // DEBUG --- create bounding boxes

    // read tree buffer into nodes vector
    std::vector<GLuint> nodes(nodeOffset);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, nodeOffset * sizeof(GLuint), nodes.data());

    m_voxel_bboxes.clear();
    m_voxel_bboxes.reserve(nodeOffset);
    std::pair<GLuint, core::AABB> stack[128]; // >= vars.octree_tree_levels
    stack[0] = std::make_pair(0u, m_scene_bbox); // root box
    std::size_t top = 1;
    do {
        top--;
        const auto idx = stack[top].first;
        const auto bbox = stack[top].second;

        const auto childidx = nodes[idx];
        if (childidx == 0x80000000) {
            // flagged as non empty leaf
            m_voxel_bboxes.emplace_back(bbox);

        } else if ((childidx & 0x80000000) != 0) {
            // flagged parent node
            const auto baseidx = uint(childidx & 0x7FFFFFFFu); // ptr to children
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

void Renderer::render(const unsigned int tree_levels, const bool renderBBoxes,
                      const bool renderOctree, const bool debug_output)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    if (m_rebuildTree) {

        createVoxelList(debug_output);

        buildVoxelTree(debug_output);

        m_rebuildTree = false;

    }

    const auto* cam = core::res::cameras->getDefaultCam();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    //GLuint prog = m_earlyz_prog;
    //GLuint vao = 0;
    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    glUseProgram(m_vertexpulling_prog);
    glBindVertexArray(m_vertexpulling_vao);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        // bind textures
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasDiffuseTexture()) {
            int unit = core::bindings::DIFFUSE_TEX_UNIT;
            GLuint tex = *mat->getDiffuseTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasSpecularTexture()) {
            int unit = core::bindings::SPECULAR_TEX_UNIT;
            GLuint tex = *mat->getSpecularTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasGlossyTexture()) {
            int unit = core::bindings::GLOSSY_TEX_UNIT;
            GLuint tex = *mat->getGlossyTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasNormalTexture()) {
            int unit = core::bindings::NORMAL_TEX_UNIT;
            GLuint tex = *mat->getNormalTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasEmissiveTexture()) {
            int unit = core::bindings::EMISSIVE_TEX_UNIT;
            GLuint tex = *mat->getEmissiveTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAlphaTexture()) {
            int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = *mat->getAlphaTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (mat->hasAmbientTexture()) {
            int unit = core::bindings::AMBIENT_TEX_UNIT;
            GLuint tex = *mat->getAmbientTexture();
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }

        //glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
        //        cmd.indices, 1, cmd.basevertex, cmd.instance->getIndex());
        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, 0, cmd.instance->getIndex());
    }

    if (renderBBoxes)
        renderBoundingBoxes();

    if (renderOctree)
        debugRenderTree();
}

/****************************************************************************/

void Renderer::renderBoundingBoxes()
{
    if (m_geometry.empty())
        return;

    core::res::instances->bind();

    GLuint prog = m_bbox_prog;
    glUseProgram(prog);
    glBindVertexArray(m_bbox_vao);

    glDisable(GL_DEPTH_TEST);

    for (const auto* g : m_geometry) {
        glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE,
                nullptr, 1, g->getMesh()->basevertex(), g->getIndex());
    }
}

/****************************************************************************/

void Renderer::initBBoxStuff()
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

void Renderer::debugRenderTree()
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
