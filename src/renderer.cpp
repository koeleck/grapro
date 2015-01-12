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

namespace
{

constexpr int FLAG_PROG_LOCAL_SIZE = 64;
constexpr int ALLOC_PROG_LOCAL_SIZE = 256;

} // anonymous namespace

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
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(FLAG_PROG_LOCAL_SIZE));
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(ALLOC_PROG_LOCAL_SIZE));
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

    // debug render
    core::res::shaders->registerShader("octreeDebugBBox_vert", "tree/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("octreeDebugBBox_frag", "tree/bbox.frag", GL_FRAGMENT_SHADER);
    m_voxel_bbox_prog = core::res::shaders->registerProgram("octreeDebugBBox_prog",
            {"octreeDebugBBox_vert", "octreeDebugBBox_frag"});

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
    //glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // node buffer
    unsigned int max_num_nodes = 1;
    unsigned int tmp = 1;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {
        tmp *= 8;
        max_num_nodes += tmp;
    }
    unsigned int mem = max_num_nodes * 4;
    std::string unit = "B";
    if (mem > 1024) {
        unit = "kiB";
        mem /= 1024;
    }
    if (mem > 1024) {
        unit = "MiB";
        mem /= 1024;
    }
    if (mem > 1024) {
        unit = "GiB";
        mem /= 1024;
    }
    LOG_INFO("max nodes: ", max_num_nodes, " (", mem, unit, ")");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, max_num_nodes * sizeof(GLuint), nullptr, GL_MAP_READ_BIT);
    //glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // Atomic counter
    // The first GLuint is for voxel fragments
    // The second GLuint is for nodes
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, 2 * sizeof(GLuint), nullptr, GL_MAP_READ_BIT);
    //glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
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

    LOG_INFO("###########################\n"
             "# Voxel Fragment Creation #\n"
             "###########################");

    m_voxelize_timer->start();

    // TODO Use empty framebuffer (https://www.opengl.org/wiki/Framebuffer_Object#Empty_framebuffers)
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
    glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint),
            GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // setup
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, 0, sizeof(GLuint));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    renderGeometry(voxel_prog);

    // TODO move to shader
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &m_numVoxelFrag);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    m_voxelize_timer->stop();

    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);


    LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag);

}

/****************************************************************************/

void Renderer::buildVoxelTree()
{

    LOG_INFO("###########################\n"
             "#     Octree Creation     #\n"
             "###########################");

    m_tree_timer->start();

    // calculate max invocations for compute shader to get all the voxel fragments
    const unsigned int flag_num_workgroups = (m_numVoxelFrag + FLAG_PROG_LOCAL_SIZE - 1) / FLAG_PROG_LOCAL_SIZE;

    GLint loc;
    const GLuint flag_prog = m_octreeNodeFlag_prog;
    const GLuint alloc_prog = m_octreeNodeAlloc_prog;

    // uniforms
    loc = glGetUniformLocation(flag_prog, "uNumVoxelFrag");
    glProgramUniform1ui(flag_prog, loc, m_numVoxelFrag);
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "uTreeLevels");
    glProgramUniform1ui(flag_prog, loc, vars.voxel_octree_levels);

    GLint uMaxLevel = glGetUniformLocation(flag_prog, "uMaxLevel");
    GLint uStartNode = glGetUniformLocation(alloc_prog, "uStartNode");
    GLint uCount = glGetUniformLocation(alloc_prog, "uCount");

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, sizeof(GLuint), sizeof(GLuint));

    unsigned int previously_allocated = 8; // only root node was allocated
    unsigned int numAllocated = 8; // we're only allocating one block of 8 nodes, so yeah, 8;
    unsigned int start_node = 0;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        LOG_INFO("");
        LOG_INFO("Starting with max level ", i);

        /*
         *  flag nodes
         */
        if (i == 0) {
            // rootnode will always be subdivided, so don't run
            // the compute shader, but flag the first node
            // with glClearBufferSubData
            const GLuint flag = 0x80000000;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(GLuint),
                GL_RED_INTEGER, GL_UNSIGNED_INT, &flag);

            // clear the remaining 7 nodes
            const GLuint zero = 0;
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, sizeof(GLuint),
                7 * sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

            // set the allocation counter to 1, so we don't overwrite the root node
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
            const GLuint one = 1;
            glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, sizeof(GLuint), sizeof(GLuint),
                GL_RED_INTEGER, GL_UNSIGNED_INT, &one);

        } else {
            glUseProgram(flag_prog);

            glProgramUniform1ui(flag_prog, uMaxLevel, i);

            glDispatchCompute(flag_num_workgroups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        /*
         *  allocate child nodes
         */
        glUseProgram(alloc_prog);

        glProgramUniform1ui(alloc_prog, uCount, previously_allocated);
        glProgramUniform1ui(alloc_prog, uStartNode, start_node);

        GLuint alloc_workgroups = (previously_allocated + ALLOC_PROG_LOCAL_SIZE - 1) / ALLOC_PROG_LOCAL_SIZE;
        glDispatchCompute(alloc_workgroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        // start_node points to the first node that was allocated in this iteration
        start_node += previously_allocated;

        /*
         *  How many child node were allocated?
         */
        GLuint counter_value;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), sizeof(GLuint), &counter_value);
        counter_value *= 8;
        previously_allocated = counter_value - numAllocated;
        numAllocated = counter_value;

        LOG_INFO(" num allocated this iterator: ", previously_allocated);
    }
    m_tree_timer->stop();

    LOG_INFO("\nTotal Nodes created: ", numAllocated);

    // TODO
    /*
     *  flag non-empty leaf nodes
     */
    //LOG_INFO("flagging non-empty leaf nodes...");
    //glUseProgram(m_octreeNodeFlag_prog);

    //// uniforms
    //loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
    //glUniform1ui(loc, m_numVoxelFrag);
    //loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_treeLevels");
    //glUniform1ui(loc, vars.voxel_octree_levels);
    //loc = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");
    //glUniform1ui(loc, vars.voxel_octree_levels);

    //// dispatch
    //LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
    //LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
    //glDispatchCompute(groupDimX, groupDimY, 1);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    ///*
    // *  write information to leafs
    // */

    //LOG_INFO("\nfilling leafs...");
    //glUseProgram(m_octreeLeafStore_prog);

    //// uniforms
    //loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_numVoxelFrag");
    //glUniform1ui(loc, m_numVoxelFrag);
    //loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_treeLevels");
    //glUniform1ui(loc, vars.voxel_octree_levels);
    //loc = glGetUniformLocation(m_octreeLeafStore_prog, "u_maxLevel");
    //glUniform1ui(loc, vars.voxel_octree_levels);

    //// dispatch
    //LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
    //LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
    //glDispatchCompute(groupDimX, groupDimY, 1);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // DEBUG --- create bounding boxes
    std::vector<GLuint> nodes(numAllocated);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numAllocated * sizeof(GLuint), nodes.data());

    //for (auto idx : nodes) {
    //    bool flag = idx & 0x80000000u;
    //    GLuint n = idx & 0x7FFFFFFFu;
    //    LOG_INFO(" [", flag, "] ", n);
    //}

    m_voxel_bboxes.clear();
    m_voxel_bboxes.reserve(numAllocated);
    std::pair<GLuint, core::AABB> stack[128];
    stack[0] = std::make_pair(0u, m_scene_bbox);
    std::size_t top = 1;
    do {
        top--;
        const auto idx = stack[top].first;
        const auto bbox = stack[top].second;

        const auto childidx = nodes[idx];
        if ((childidx & 0x80000000) != 0) {
            m_voxel_bboxes.emplace_back(bbox);

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

void Renderer::render(const bool renderBBoxes, const bool renderOctree)
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
    if (renderOctree)
        debugRenderTree();
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

void Renderer::debugRenderTree()
{
    if (m_voxel_bboxes.empty())
        return;

    GLuint prog = m_voxel_bbox_prog;
    glUseProgram(prog);
    glBindVertexArray(m_bbox_vao);

    glEnable(GL_DEPTH_TEST);

    for (const auto& bbox : m_voxel_bboxes) {
        float data[6] = {bbox.pmin.x, bbox.pmin.y, bbox.pmin.z,
                         bbox.pmax.x, bbox.pmax.y, bbox.pmax.z};
        glUniform3fv(0, 2, data);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr);

        //glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE,
        //        nullptr, 1, g->getMesh()->basevertex(), g->getIndex());
    }
}

/****************************************************************************/
