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

    core::res::shaders->registerShader("octreeNodeInitComp", "tree/nodeinit.comp", GL_COMPUTE_SHADER);
    m_octreeNodeInit_prog = core::res::shaders->registerProgram("octreeNodeInit_prog", {"octreeNodeInitComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

    m_voxelize_timer = m_timers.addGPUTimer("Voxelize");
    m_tree_timer = m_timers.addGPUTimer("Octree");

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);

    m_voxelize_cam = core::res::cameras->createOrthogonalCam("voxelization_cam",
            glm::dvec3(0.0), glm::dvec3(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));


    // voxel buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelStruct), nullptr, GL_MAP_READ_BIT);
    const GLuint zero = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    // node buffer
    const auto dim = static_cast<int>(std::pow(2, vars.voxel_octree_levels));
    const auto max_nodes = static_cast<unsigned int>(dim * dim * dim / 4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, max_nodes * sizeof(GLuint), nullptr, GL_MAP_READ_BIT);
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);

    // Atomic counter
    // The first GLuint is for voxel fragments
    // The second GLuint is for nodes
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, 2 * sizeof(GLuint), nullptr, GL_MAP_READ_BIT);
    glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);
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

    m_voxelize_cam->setLeft(m_scene_bbox.pmin.x);
    m_voxelize_cam->setRight(m_scene_bbox.pmax.x);
    m_voxelize_cam->setBottom(m_scene_bbox.pmin.y);
    m_voxelize_cam->setTop(m_scene_bbox.pmax.y);
    m_voxelize_cam->setZNear(m_scene_bbox.pmin.z);
    m_voxelize_cam->setZFar(m_scene_bbox.pmax.z);
    // set view matrix to identity
    m_voxelize_cam->setPosition(glm::dvec3(0.));
    m_voxelize_cam->setOrientation(glm::dquat());
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));

}

/****************************************************************************/

void Renderer::createVoxelList()
{

    LOG_INFO("");
    LOG_INFO("###########################");
    LOG_INFO("# Voxel Fragment Creation #");
    LOG_INFO("###########################");

    m_voxelize_timer->start();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto dim = static_cast<int>(std::pow(2, vars.voxel_octree_levels));
    glViewport(0, 0, dim, dim);

    auto* old_cam = core::res::cameras->getDefaultCam();
    core::res::cameras->makeDefault(m_voxelize_cam);


    GLuint voxel_prog = m_voxel_prog;

    GLint loc;
    loc = glGetUniformLocation(voxel_prog, "u_width");
    glProgramUniform1i(voxel_prog, loc, dim);
    loc = glGetUniformLocation(voxel_prog, "u_height");
    glProgramUniform1i(voxel_prog, loc, dim);

    // reset counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    const GLuint zero = 0;
    glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, 0, sizeof(GLuint));

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    renderGeometry(voxel_prog);

    // TODO move to shader
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    m_numVoxelFrag = *static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_READ_ONLY));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);


    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);

    m_voxelize_timer->stop();

    LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag);
    LOG_INFO("");

}

/****************************************************************************/

void Renderer::buildVoxelTree()
{

    LOG_INFO("");
    LOG_INFO("###########################");
    LOG_INFO("#     Octree Creation     #");
    LOG_INFO("###########################");

    m_tree_timer->start();

    // calculate max invocations for compute shader to get all the voxel fragments
    const auto dataWidth = 1024u;
    unsigned int dataHeight = (m_numVoxelFrag + dataWidth - 1) / dataWidth;
    const unsigned int groupDimX = dataWidth / 8;
    const unsigned int groupDimY = (dataHeight + 7) / 8;


    GLint loc;

    unsigned int nodeOffset = 0; // offset to first node of next level
    unsigned int allocOffset = 1; // offset to first free space for new children
    std::vector<unsigned int> allocList(1, 1); // number of nodes in each tree level; root level = 1

    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        LOG_INFO("");
        LOG_INFO("Starting with max level ", i);

        /*
         *  flag nodes
         */

        glUseProgram(m_octreeNodeFlag_prog);

        // uniforms
        loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
        glUniform1ui(loc, m_numVoxelFrag);
        loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_treeLevels");
        glUniform1ui(loc, vars.voxel_octree_levels);
        loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");
        glUniform1ui(loc, i);

        // dispatch
        LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
        LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
        glDispatchCompute(groupDimX, groupDimY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /*
         *  allocate child nodes
         */

        glUseProgram(m_octreeNodeAlloc_prog);

        const unsigned int numThreads = allocList[i]; // number of child nodes to be allocated at this level

        // uniforms
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_num");
        glUniform1ui(loc, numThreads);
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_start");
        glUniform1ui(loc, nodeOffset);
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_allocStart");
        glUniform1ui(loc, allocOffset);

        // dispatch
        const unsigned int allocwidth = 64;
        const unsigned int allocGroupDim = (allocList[i]+allocwidth-1)/allocwidth;
        LOG_INFO("Dispatching NodeAlloc with ", allocGroupDim, "*1*1 groups with 64*1*1 threads each");
        LOG_INFO("--> ", allocGroupDim * 64, " threads");
        glDispatchCompute(allocGroupDim, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        /*
         *  get how many child nodes have to be allocated
         */

        GLuint childNodesToBeAllocated;
        const GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &childNodesToBeAllocated);
        glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); //reset counter to zero
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        LOG_INFO(childNodesToBeAllocated, " child nodes have been allocated");

        /*
         *  init child nodes
         */

        // DO WE REALLY NEED THIS ????

        /*glUseProgram(m_octreeNodeInit_prog);

        */const unsigned int nodeAllocated = childNodesToBeAllocated * 8;/*

        // uniforms
        loc = glGetUniformLocation(m_octreeNodeInit_prog, "u_num");
        glUniform1ui(loc, nodeAllocated);
        loc = glGetUniformLocation(m_octreeNodeInit_prog, "u_allocStart");
        glUniform1ui(loc, allocOffset);

        dataHeight = (nodeAllocated + dataWidth - 1) / dataWidth;
        const unsigned int initGroupDimX = dataWidth / 8;
        const unsigned int initGroupDimY = (dataHeight + 7) / 8;

        // dispatch
        LOG_INFO("Dispatching NodeInit with ", initGroupDimX, "*", initGroupDimY, "*1 groups with 8*8*1 threads each");
        LOG_INFO("--> ", initGroupDimX * initGroupDimY * 64, " threads");
        glDispatchCompute(initGroupDimX, initGroupDimY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);*/

        /*
         *  update offsets
         */

        allocList.emplace_back(nodeAllocated);  // nodeAllocated is the number of threads
                                                // we want to launch in the next level
        nodeOffset += allocList[i]; // add number of newly allocated child nodes to offset
        allocOffset += nodeAllocated;

    }

    LOG_INFO("");
    LOG_INFO("Total Nodes created: ", nodeOffset);

    m_tree_timer->stop();

    /*
     *  flag non-empty leaf nodes
     */
    LOG_INFO("flagging non-empty leaf nodes...");
    glUseProgram(m_octreeNodeFlag_prog);

    // uniforms
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
    glUniform1ui(loc, m_numVoxelFrag);
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_treeLevels");
    glUniform1ui(loc, vars.voxel_octree_levels);
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");
    glUniform1ui(loc, vars.voxel_octree_levels);

    // dispatch
    LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
    LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
    glDispatchCompute(groupDimX, groupDimY, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /*
     *  write information to leafs
     */

    LOG_INFO("\nfilling leafs...");
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
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/****************************************************************************/

void Renderer::render(const bool renderBBoxes)
{
    if (m_geometry.empty())
        return;

    if (m_rebuildTree) {
        createVoxelList();
        buildVoxelTree();
        m_rebuildTree = false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    renderGeometry(m_vertexpulling_prog);

    if (renderBBoxes)
        renderBoundingBoxes();
}

/****************************************************************************/

void Renderer::renderGeometry(const GLuint prog)
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
