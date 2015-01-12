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

    m_render_timer = m_timers.addGPUTimer("Octree");

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
    glUniform1i(loc, dim);
    loc = glGetUniformLocation(m_voxel_prog, "u_height");
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

    /*
     *  case: assuming always 8 child nodes will be created, assume voxelFrags = 200000:            ////only in one half (posx = [0,127])
     *
     *      groupDimX = 128
     *      groupDimY = 25
     *
     *      nodeOffset = 0
     *      allocOffset = 1
     *      maxNodesPerLevel[0] = 1
     *
     *      at i = 0:
     *
     *          flagShader:
     *              u_numVoxelFrag = 200000
     *              u_treeLevels = 8 (vars.voxel_octree_levels)
     *              u_maxLevel = 0
     *
     *              threadId = gl_GlobalInvocationID.y
     *                         * gl_NumWorkGroups.x * gl_WorkGroupSize.x
     *                         + gl_GlobalInvocationID.x
     *                       = (gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y)
     *                         * gl_NumWorkGroups.x * gl_WorkGroupSize.x
     *                         + gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x
     *                       = ([0, gl_NumWorkGroups.y) * local_size_y + [0, local_size_y))
     *                         * gl_NumWorkGroups.x * local_size_x
     *                         + [0, gl_NumWorkGroups.x) * local_size_x + [0, local_size_x)
     *                       = ([0, groupDimY) * local_size_y + [0, local_size_y))
     *                         * groupDimX * local_size_x
     *                         + [0, groupDimX) * local_size_x + [0, local_size_x)
     *                       = ([0,24] * 8 + [0,7]) * 128 * 8 + [0,127] * 8 + [0,7]
     *                       = [0,199] * 1024 + [0,1016] + [0,7]
     *                       = [0,204799]
     *              threadID = [0,199999]
     *
     *              no iterations through for loop
     *              childIdx = 0
     *              nodePtr = 0 + MSB
     *              octree[0].id = 0 + MSB
     *
     *          allocwidth = 64
     *          allocGroupDim = 1
     *
     *          allocShader:
     *              u_numNodesThisLevel = 1
     *              u_nodeOffset = 0
     *              u_allocOffset = 1
     *              threadID = gl_GlobalInvocationID.x
     *                       = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x
     *                       = [0, gl_NumWorkGroups.x) * local_size_x + [0, local_size_x)
     *                       = [0, allocGroupDim) * 64 + [0, 64)
     *                       = [0, 1) * 64 + [0, 64)
     *                       = [0, 63]
     *
     *              threadID = 0
     *              childIdx = octree[0].id = 0 + MSB
     *              MSB is set! -> counter = 1
     *              off = (0 * 8) + 1 + MSB
     *              octree[0].id = 1 + MSB
     *
     *          actualNodesAllocated = 1
     *          maxNodesToBeAllocated = 8
     *
     *          maxNodesPerLevel[1] = 8
     *          nodeOffset = 1
     *          allocOffset = 9
     *
     *      at i = 1:
     *
     *          flagShader:
     *              u_maxLevel = 1
     *              threadID = [0,199999]
     *
     *              childIdx = 0
     *              nodePtr = octree[0].id = 1 + MSB
     *              subNodePtr = 0
     *              voxelDim = 256
     *              umin = 0,0,0
     *              umax = 256,256,256
     *
     *              one iteration through for loop:
     *                  voxelDim = 128
     *                  childIdx = 1
     *                  subNodePtrX = clamp(1 + [0,255] - 0 - 128, 0, 1) = clamp([-127,128], 0, 1)  ////subNodePtrX = clamp(1 + [0,127] - 0 - 128, 0, 1) = 0
     *                              = [0,1] = subNodePtrY,Z                                         ////subNodePtrYZ = clamp(1 + [0,255] - 0 - 128, 0, 1) = [0,1]
     *                  subNodePtr = [0,1] + 4 * [0,1] + 2 * [0,1] = [0,7]                          ////subNodePtr = 0 + 4 * [0,1] + 2 * [0,1] = [0;2;4;6]
     *                  childIdx = [1,8]                                                            ////childIdx = [1;3;5;7]
     *
     *                  nodePtr = octree[[1,8]].id = 0                                              ////nodePtr = octree[[1;3;5;7]].id = 0
     *
     *              nodePtr = 0 + MSB
     *              octree[[1,8]] = 0 + MSB                                                         ////octree[[1;3;5;7]] = 0 + MSB
     *
     *          allocwidth = 64
     *          allocGroupDim = (maxNodesPerLevel[1] + allocwidth - 1) / allocwidth = 71 / 64 = 1
     *
     *          allocShader:
     *              u_numNodesThisLevel = 8
     *              u_nodeOffset = 1
     *              u_allocOffset = 9
     *              gl_GlobalInvocationID.x = [0, allocGroupDim) * 64 + [0, 64) = [0,63]
     *
     *              threadID = [0,7]
     *              childIdx = octree[[1,8]].id = 0 + MSB                                           ////childIdx = octree[[1,8]].id = 0 + MSB for [0;2;4;6] && 0 for [1;3;5;7]
     *              MSBs are set! -> counter = 8                                                    ////MSBs are set for [0;2;4;6] -> counter = 4
     *              off = [0,7] * 8 + 9 + MSB = [9;17;...;65] + MSB                                 ////off = [0,3] * 8 + 9 + MSB = [9;17;25;33] + MSB
     *              octree[1] = 9 + MSB                                                             ////octree[1] = 9 + MSB
     *              octree[2] = 17 + MSB                                                            ////octree[3] = 17 + MSB
     *              ...                                                                             ////octree[5] = 25 + MSB
     *              octree[8] = 65 + MSB                                                            ////octree[7] = 33 + MSB
     *
     *          actualNodesAllocated = 8                                                            ////actualNodesAllocated = 4
     *          maxNodesToBeAllocated = 64                                                          ////maxNodesToBeAllocated = 32
     *
     *          maxNodesPerLevel[2] = 64                                                            ////maxNodesPerLevel[2] = 32
     *          nodeOffset = nodeOffset + maxNodesPerLevel[1] = 1 + 8 = 9
     *          allocOffset = allocOffset + maxNodesToBeAllocated = 9 + 64 = 73                     ////allocOffset = allocOffset + maxNodesToBeAllocated = 9 + 32 = 41
     *
     *      at i = 2:
     *
     *          flagShader:
     *              u_maxLevel = 2
     *              threadID = [0,199999]
     *
     *              childIdx = 0
     *              nodePtr = octree[0].id = 1 + MSB
     *              subNodePtr = 0
     *              voxelDim = 256
     *              umin = 0,0,0
     *              umax = 256,256,256
     *
     *              two iterations through loop:
     *                  voxelDim = 128
     *                  childIdx = 1
     *                  subNodePtrX = clamp(1 + [0,255] - 0 - 128, 0, 1) = [0,1] = subNodePtrY,Z    ////subNodePtrX = clamp(1 + [0,127] - 0 - 128, 0, 1) = 0
     *                      pos.x = [0,127] -> 0                                                    ////subNodePtrYZ = clamp(1 + [0,255] - 0 - 128, 0, 1) = [0,1]
     *                      pos.x = [128,255] -> 1
     *                  subNodePtr = [0,7]                                                          ////subNodePtr = 0 + 4 * [0,1] + 2 * [0,1] = [0;2;4;6]
     *                  childIdx = [1,8]                                                            ////childIdx = [1;3;5;7]
     *
     *                  umin.x = 128 * [0,1] = [0;128] = umin.yz                                    ////umin.x = 0 + 128 * 0 = 0
     *                      pos = [0,127] -> 0                                                      ////umin.yz = 0 + 128 * [0,1] = [0;128]
     *                      pos = [128,255] -> 128
     *                  nodePtr = octree[[1,8]].id = [9;17;...;65] + MSB                            ////nodePtr = octree[[1;3;5;7]].id = [9;17;25;33] + MSB
     *
     *                  voxelDim = 64
     *                  childIdx = [9;17;...;65]                                                    ////childIdx = [9;17;25;33]
     *                  subNodePtrXYZ = clamp(1 + pos - umin - voxelDim, 0, 1) = [0,1]
     *                  subNodePtr = [0,7]                                                          ////subNodePtr = [0,4]
     *                  childIdx = [9,72]                                                           ////childIdx = [9,12;17,20;25,28;33,36]
     *
     *                  nodePtr = octree[[9,72]].id = 0                                             ////nodePtr = octree[[9,12;17,20;25,28;33,36]].id = 0
     *
     *              nodePtr = 0 + MSB
     *              octree[[9,72]].id = 0 + MSB;                                                    ////octree[[9,12;17,20;25,28;33,36]].id = 0 + MSB;
     *
     *          allocWidth = 64
     *          allocGroupDim = (maxNodesPerLevel[2] + allocwidth - 1) / allocwidth = (64 + 64 - 1) / 64 = 1
     *
     *          allocShader:
     *              u_numNodesThisLevel = 64                                                        ////u_numNodesThisLevel = 32
     *              u_nodeOffset = 9
     *              u_allocOffset = 73                                                              ////u_allocOffset = 41
     *              gl_GlobalInvocationID.x = [0, allocGroupDim) * 64 + [0, 64) = [0,63]
     *
     *              threadID = [0,63]                                                               ////threadID = [0,31]
     *              childIdx = octree[[9,72]].id = 0 + MSB                                          ////childIdx = octree[[9,40]].id = 0 + MSB for [9,12;17,20;25,28;33,36] && 0 else
     *              MSBs are set! -> counter = 64                                                   ////counter = 16
     *              off = [0,63] * 8 + 73 + MSB = [73;81;...;577] + MSB                             ////off = [0,15] * 8 + 41 + MSB = [41;49;...;161] + MSB
     *              octree[9].id = 73 + MSB                                                         ////octree[[9,12]].id = [41;49;57;65] + MSB
     *              octree[10].id = 81 + MSB                                                        ////octree[[17,20]].id = [73;81;89;97] + MSB
     *              ...                                                                             ////octree[[25,28]].id = [105;113;121;129] + MSB
     *              octree[72].id = 577 + MSB                                                       ////octree[[33,36]].id = [137;145;153;161] + MSB
     *
     *          actualNodesAllocated = 64                                                           ////actualNodesAllocated = 16
     *          maxNodesToBeAllocated = actualNodesAllocated * 8 = 512                              ////maxNodesToBeAllocated = actualNodesAllocated * 8 = 128
     *
     *          maxNodesPerLevel[3] = 512                                                              ////maxNodesPerLevel[3] = 128
     *          nodeOffset = 9 + 64 = 73                                                            ////nodeOffset = 9 + 32 = 41
     *          allocOffset = allocOffset + maxNodesToBeAllocated = 73 + 512 = 585                  ////allocOffset = 41 + 128 = 169
     *
     *      at i = 3:
     *
     *          flagShader:
     *              u_maxLevel = 3
     *              threadID = [0,199999]
     *
     *              childIdx = 0
     *              nodePtr = octree[0].id = 1 + MSB
     *              subNodePtr = 0
     *              voxelDim = 256
     *              umin = 0,0,0
     *              umax = 256,256,256
     *
     *              three iterations through loop:
     *                  voxelDim = 128
     *                  childIdx = 1
     *                  subNodePtrX = clamp(1 + [0,255] - 0 - 128, 0, 1) = [0,1] = subNodePtrY,Z
     *                      pos.x = [0,127] -> 0
     *                      pos.x = [128,255] -> 1
     *                  subNodePtr = [0,7]
     *                  childIdx = [1,8]                                                            ////
     *
     *                  umin.x = 128 * [0,1] = [0;128] = umin.yz                                    ////
     *                      pos = [0,127] -> 0
     *                      pos = [128,255] -> 128
     *                  nodePtr = octree[[1,8]].id = [9;17;...;65] + MSB                            ////
     *
     *                  voxelDim = 64
     *                  childIdx = [9;17;...;65]                                                    ////
     *                  subNodePtrXYZ = clamp(1 + pos - umin - voxelDim, 0, 1) = [0,1]
     *                  subNodePtr = [0,7]                                                          ////
     *                  childIdx = [9,72]                                                           ////
     *
     *                  umin.x = [0;128] + 64 * [0,1] = [0;64;128;192] = umin.yz
     *                  nodePtr = octree[[9,72]].id = [73;81;...;577] + MSB                         ////
     *
     *                  voxelDim = 32
     *                  childIdx = [73;81;...;577]
     *                  subNodePtrXYZ = clamp(1 + [0,255] - [0;64;128;192] - 32, 0, 1) = [0,1]
     *                  subNodePtr = [0,7]
     *                  childIdx = [73,584]
     *
     *                  nodePtr = octree[[73,584]].id = 0
     *
     *              nodePtr = 0 + MSB
     *              octree[[73,584]].id = 0 + MSB;                                                  ////
     *
     *          allocWidth = 64
     *          allocGroupDim = (maxNodesPerLevel[3] + allocwidth - 1) / allocwidth = (512 + 64 - 1) / 64 = 8
     *
     *          allocShader:
     *              u_numNodesThisLevel = 512                                                       ////
     *              u_nodeOffset = 73
     *              u_allocOffset = 585                                                             ////
     *              gl_GlobalInvocationID.x = [0, allocGroupDim) * 64 + [0, 64)
     *                                      = [0,7] * 64 + [0,63]
     *                                      = [0,511]
     *
     *              threadID = [0,511]                                                              ////
     *              childIdx = octree[[73,584]].id = 0 + MSB                                        ////
     *              MSBs are set! -> counter = 512                                                  ////
     *              off = [0,511] * 8 + 585 + MSB = [585;593;...;4673] + MSB                        ////
     *              octree[73].id = 585 + MSB                                                       ////
     *              octree[74].id = 593 + MSB                                                       ////
     *              ...                                                                             ////
     *              octree[584].id = 4673 + MSB                                                     ////
     *
     *          actualNodesAllocated = 512                                                          ////
     *          maxNodesToBeAllocated = actualNodesAllocated * 8 = 4096                             ////
     *
     *          maxNodesPerLevel[4] = 4096                                                             ////
     *          nodeOffset = 73 + 512 = 585                                                         ////
     *          allocOffset = allocOffset + maxNodesToBeAllocated = 585 + 4096 = 4681               ////
     *
     */

    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        LOG_INFO("");
        LOG_INFO("Starting with max level ", i);

        /*
         *  flag nodes
         */

        glUseProgram(m_octreeNodeFlag_prog);

        // uniforms
        glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_maxLevel, i);

        // dispatch
        LOG_INFO("Dispatching NodeFlag with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
        LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
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
        LOG_INFO("Dispatching NodeAlloc with ", allocGroupDim, "*1*1 groups with 64*1*1 threads each");
        LOG_INFO("--> ", allocGroupDim * 64, " threads");
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
        LOG_INFO(actualNodesAllocated, " nodes have been allocated");
        const unsigned int maxNodesToBeAllocated = actualNodesAllocated * 8;

        /*
         *  update offsets
         */

        maxNodesPerLevel.emplace_back(maxNodesToBeAllocated);   // maxNodesToBeAllocated is the number of threads
                                                                // we want to launch in the next level
        nodeOffset += maxNodesPerLevel[i];                      // add number of newly allocated child nodes to offset
        allocOffset += maxNodesToBeAllocated;

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
