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

Renderer::Renderer()
  : m_numVoxelFrag{0u},
    m_rebuildTree{true}
{
    // create programs
    /*for (unsigned char i = 0; i < 8; ++i) {
        util::bitfield<core::MeshComponents> c(i);
        std::string name_ext = "pos";
        std::string defines;
        if (c & core::MeshComponents::Normals) {
            name_ext += "_normals";
            defines = "HAS_NORMALS";
        }
        if (c & core::MeshComponents::TexCoords) {
            name_ext += "_texcoords";
            if (!defines.empty())
                defines += ',';
            defines += "HAS_TEXCOORDS";
        }
        if (c & core::MeshComponents::Tangents) {
            name_ext += "_tangents";
            if (!defines.empty())
                defines += ',';
            defines += "HAS_TANGENTS";
        }
        const auto vert = "basic_vert_" + name_ext;
        const auto frag = "basic_frag_" + name_ext;
        core::res::shaders->registerShader(vert, "basic/basic.vert", GL_VERTEX_SHADER,
                defines);
        core::res::shaders->registerShader(frag, "basic/basic.frag", GL_FRAGMENT_SHADER,
                defines);
        auto prog = core::res::shaders->registerProgram("prog_" + vert + frag,
                {vert, frag});
        m_programs.emplace(c(), prog);
    }
    core::res::shaders->registerShader("noop_frag", "basic/noop.frag", GL_FRAGMENT_SHADER);
    m_earlyz_prog = core::res::shaders->registerProgram("early_z_prog",
            {"basic_vert_pos", "noop_frag"});*/

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

    m_render_timer = m_timers.addGPUTimer("Octree");

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);

    m_voxelize_cam = core::res::cameras->createOrthogonalCam("voxelization_cam",
            glm::dvec3(0.0), glm::dvec3(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));
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

}

/****************************************************************************/

void Renderer::genAtomicBuffer()
{
    GLuint initVal = 0;

    if(m_atomicCounterBuffer)
        glDeleteBuffers(1, &m_atomicCounterBuffer);

    glGenBuffers(1, &m_atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);

    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initVal, GL_STATIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

/****************************************************************************/

void Renderer::genVoxelBuffer()
{
    if(m_voxelBuffer)
        glDeleteBuffers(1, &m_voxelBuffer);

    glGenBuffers(1, &m_voxelBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);

    // allocate
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelStruct), nullptr, GL_MAP_READ_BIT);

    // fill with zeroes
    const GLuint zero = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
}

/****************************************************************************/

void Renderer::genOctreeNodeBuffer(const std::size_t size)
{
    if(m_octreeNodeBuffer)
        glDeleteBuffers(1, &m_octreeNodeBuffer);

    glGenBuffers(1, &m_octreeNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);

    // allocate
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_READ_BIT);

    // fill with zeroes
    const GLuint zero = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
}

/****************************************************************************/

void Renderer::createVoxelList()
{

    LOG_INFO("");
    LOG_INFO("###########################");
    LOG_INFO("# Voxel Fragment Creation #");
    LOG_INFO("###########################");

    m_render_timer->start();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto dim = static_cast<int>(std::pow(2, vars.voxel_octree_levels));
    glViewport(0, 0, dim, dim);

    auto* old_cam = core::res::cameras->getDefaultCam();
    core::res::cameras->makeDefault(m_voxelize_cam);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glUseProgram(m_voxel_prog);

    // uniforms
    GLint loc;
    loc = glGetUniformLocation(m_voxel_prog, "u_width");
    glUniform1i(loc, vars.screen_width);
    loc = glGetUniformLocation(m_voxel_prog, "u_height");
    glUniform1i(loc, vars.screen_height);

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


    /*glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    auto vecPtr = static_cast<VoxelStruct *>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vars.max_voxel_fragments * sizeof(VoxelStruct), GL_MAP_READ_BIT));
    if (vecPtr == nullptr) LOG_INFO("NULL!");
    LOG_INFO("Position: ", vecPtr[0].position[0], " ", vecPtr[0].position[1], " ", vecPtr[0].position[2]);
    LOG_INFO("Color: ", vecPtr[0].color[0], " ", vecPtr[0].color[1], " ", vecPtr[0].color[2]);
    LOG_INFO("Normal: ", vecPtr[0].normal[0], " ", vecPtr[0].normal[1], " ", vecPtr[0].normal[2]);*/

    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);

    m_render_timer->stop();

    LOG_INFO("");

    for (const auto& t : m_timers.getTimers()) {
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                t.second->time()).count();
        LOG_INFO("Elapsed Time: ", static_cast<int>(msec), "ms");
    }
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

    m_render_timer->start();

    // calculate max invocations for compute shader to get all the voxel fragments
    const auto dataWidth = 1024u;
    unsigned int dataHeight = (m_numVoxelFrag + dataWidth - 1) / dataWidth;
    const unsigned int groupDimX = dataWidth / 8;
    const unsigned int groupDimY = (dataHeight + 7) / 8;

    // calculate max possible size of octree
    unsigned int totalNodes = 1;
    unsigned int tmp = 1;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        tmp *= 8;
        totalNodes += tmp;

    }

    // generate octree
    genOctreeNodeBuffer(totalNodes * sizeof(OctreeNodeStruct));

    // atomic counter (counts how many child node have to be allocated)
    genAtomicBuffer();

    // uniforms
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

        glUseProgram(m_octreeNodeInit_prog);

        const unsigned int nodeAllocated = childNodesToBeAllocated * 8;

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
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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

    m_render_timer->stop();

    for (const auto& t : m_timers.getTimers()) {
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                t.second->time()).count();
        LOG_INFO("Elapsed Time: ", static_cast<int>(msec), "ms");
    }

    LOG_INFO("");

}

/****************************************************************************/

void Renderer::render(const bool renderBBoxes)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    if (m_rebuildTree) {

        createVoxelList();

        buildVoxelTree();

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

    /*
    // early z
    glUseProgram(prog);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        // only fully opaque meshes:
        const auto* mat = cmd.instance->getMaterial();
        if (mat->hasAlphaTexture() || mat->getOpacity() != 1.f)
            continue;

        if (vao != cmd.vao) {
            vao = cmd.vao;
            glBindVertexArray(vao);
        }

        glDrawElementsInstancedBaseVertexBaseInstance(cmd.mode, cmd.count, cmd.type,
                cmd.indices, 1, cmd.basevertex, cmd.instance->getIndex());
    }
    */

    glUseProgram(m_vertexpulling_prog);
    glBindVertexArray(m_vertexpulling_vao);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.instance->getBoundingBox()))
            continue;

        //if (prog != cmd.prog) {
        //    prog = cmd.prog;
        //    glUseProgram(prog);
        //}
        //if (vao != cmd.vao) {
        //    vao = cmd.vao;
        //    glBindVertexArray(vao);
        //}

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
