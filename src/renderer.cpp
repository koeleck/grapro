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

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

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
    unsigned int allocOffset = 1; // offset to first free space for new nodes
    std::vector<unsigned int> nodesPerLevel(1, 1); // number of nodes in each tree level; root level = 1

    /*
     *  case: assuming always 8 child nodes will be created, assume voxelFrags = 200000:            ////only in one half (posx = [0,127])
     *
     *      groupDimX = 128
     *      groupDimY = 25
     *
     *      nodeOffset = 0
     *      allocOffset = 1
     *      nodesPerLevel[0] = 1
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
     *          dataHeight = (8 + 1024 - 1) / 1024 = 1
     *          initGroupDimX = 1024 / 8 = 128
     *          initGroupDimY = (1 + 7) / 8 = 1
     *
     *          initShader:
     *              u_maxNodesToBeAllocated = 8
     *              u_allocOffset = 1
     *              threadId = gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x
     *                       = ([0, initGroupDimY) * local_size_y + [0, local_size_y))
     *                         * initGroupDimX * local_size_x
     *                         + [0, initGroupDimX) * local_size_x + [0, local_size_x)
     *                       = ([0]) * 8 + [0,7]) * 128 * 8 + [0,127] * 8 + [0,7]
     *                       = [0;1024;...;7168] + [0;127;...;1016] + [0,7]
     *                       = [0;...;8191] // hat große Lücken! ???
     *
     *              threadID = [0,7]
     *              octree[[1,8]].id = 0 // ist schon 0 gewesen -> unnütz
     *
     *          nodesPerLevel[1] = 8
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
     *          allocGroupDim = (nodesPerLevel[1] + allocwidth - 1) / allocwidth = 71 / 64 = 1
     *
     *          allocShader:
     *              u_numNodesThisLevel = 8
     *              u_nodeOffset = 1
     *              u_allocOffset = 9
     *              gl_GlobalInvocationID.x = [0, allocGroupDim) * 64 + [0, 64) = [0,63]
     *
     *              threadID = [0,7]
     *              childIdx = octree[[1,8]].id = 0 + MSB                                           ////childIdx = octree[[1,8]].id = 0 + MSB for [1;3;5;7] && 0 for [2;4;6;8]
     *              MSBs are set! -> counter = 8                                                    ////MSBs are set for [1;3;5;7] -> counter = 4
     *              off = [0,7] * 8 + 9 + MSB = [9;17;...;65] + MSB                                 ////off = [0,3] * 8 + 9 + MSB = [9;17;25;33] + MSB
     *              octree[1] = 9 + MSB                                                             ////octree[2] = 9 + MSB
     *              octree[2] = 17 + MSB                                                            ////octree[4] = 17 + MSB
     *              ...                                                                             ////octree[6] = 25 + MSB
     *              octree[8] = 65 + MSB                                                            ////octree[8] = 33 + MSB     ???????? wieso bei 2 4 6 8 und nicht bei 1 3 5 7
     *
     *          actualNodesAllocated = 8                                                            ////actualNodesAllocated = 4
     *          maxNodesToBeAllocated = 64                                                          ////maxNodesToBeAllocated = 32
     *
     *          dataHeight = (64 + 1024 - 1) / 1024 = 1                                             ////dataHeight = (32 + 1024 - 1) / 1024 = 1
     *          initGroupDimX = 128
     *          initGroupDimY = (1 + 7) / 8 = 1
     *
     *          initShader:
     *              u_maxNodesToBeAllocated = 64                                                    ////u_maxNodesToBeAllocated = 32
     *              u_allocOffset = 9
     *              threadId = [0;...;8191] // hat große Lücken! ???
     *
     *              threadID = [0,7]
     *              octree[[9,16]].id = 0 // ist schon 0 gewesen -> unnütz
     *
     *          nodesPerLevel[2] = 64                                                               ////nodesPerLevel[2] = 32
     *          nodeOffset = nodeOffset + nodesPerLevel[1] = 1 + 8 = 9
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
     *                  nodePtr = octree[[1,8]].id = [9;17;...;65] + MSB                            ////nodePtr = octree[[1;3;5;7]].id = [9;17;25;33] + MSB // ??? stimmt nimmer , siehe oben!
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
     *          allocGroupDim = (nodesPerLevel[2] + allocwidth - 1) / allocwidth = (64 + 64 - 1) / 64 = 1
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
     *          dataHeight = (512 + 1024 - 1) / 1024 = 1                                            ////dataHeight = (128 + 1024 - 1) / 1024 = 1
     *          initGroupDimX = dataWidth / 8 = 128
     *          initGroupDimY = 1
     *
     *          initShader:
     *              u_maxNodesToBeAllocated = 512                                                   ////u_maxNodesToBeAllocated = 128
     *              u_allocOffset = 73                                                              ////u_allocOffset = 41
     *              threadId = [0;1024;...;7168] + [0;127;...;1016] + [0,7] = [0;...;8191] // hat große Lücken! ???
     *
     *              threadID = [[0,7];[127,134];[254,261];[381,388];[508,511]]                      ////threadID = [[0,7];[127]]
     *              octree[73 + threadId].id = 0 // ist schon 0 gewesen -> unnütz                   ////octree[41 + threadId].id = 0 // ist schon 0 gewesen -> unnütz
     *
     *          nodesPerLevel[3] = 512                                                              ////nodesPerLevel[3] = 128
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
     *          allocGroupDim = (nodesPerLevel[3] + allocwidth - 1) / allocwidth = (512 + 64 - 1) / 64 = 8
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
     *          dataHeight = (4096 + 1024 - 1) / 1024 = 4                                           ////
     *          initGroupDimX = dataWidth / 8 = 128
     *          initGroupDimY = 11 / 8 = 1
     *
     *          initShader:
     *              u_maxNodesToBeAllocated = 4096                                                  ////
     *              u_allocOffset = 585                                                             ////
     *              threadId = [0;1024;...;7168] + [0;127;...;1016] + [0,7] = [0;...;8191] // hat große Lücken! ???
     *
     *              threadID = [[0,7];[127,134];[254,261];[381,388];[508,511] .... bis < 4096]      ////
     *              octree[585 + threadId].id = 0 // ist schon 0 gewesen -> unnütz                  ////
     *
     *          nodesPerLevel[4] = 4096                                                             ////
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

        const unsigned int numNodesThisLevel = nodesPerLevel[i]; // number of child nodes to be allocated at this level

        // uniforms
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_numNodesThisLevel");
        glUniform1ui(loc, numNodesThisLevel);
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_nodeOffset");
        glUniform1ui(loc, nodeOffset);
        loc = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_allocOffset");
        glUniform1ui(loc, allocOffset);

        // dispatch
        const unsigned int allocwidth = 64;
        const unsigned int allocGroupDim = (nodesPerLevel[i] + allocwidth - 1) / allocwidth;
        LOG_INFO("Dispatching NodeAlloc with ", allocGroupDim, "*1*1 groups with 64*1*1 threads each");
        LOG_INFO("--> ", allocGroupDim * 64, " threads");
        glDispatchCompute(allocGroupDim, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        /*
         *  get how many child nodes have to be allocated
         */

        GLuint actualNodesAllocated;
        const GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &actualNodesAllocated);
        glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); //reset counter to zero
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        LOG_INFO(actualNodesAllocated, " nodes have been allocated");

        /*
         *  init child nodes
         */

        // DO WE REALLY NEED THIS ????

        glUseProgram(m_octreeNodeInit_prog);

        const unsigned int maxNodesToBeAllocated = actualNodesAllocated * 8;

        // uniforms
        loc = glGetUniformLocation(m_octreeNodeInit_prog, "u_maxNodesToBeAllocated");
        glUniform1ui(loc, maxNodesToBeAllocated);
        loc = glGetUniformLocation(m_octreeNodeInit_prog, "u_allocOffset");
        glUniform1ui(loc, allocOffset);

        dataHeight = (maxNodesToBeAllocated + dataWidth - 1) / dataWidth;
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

        nodesPerLevel.emplace_back(maxNodesToBeAllocated);  // maxNodesToBeAllocated is the number of threads
                                                // we want to launch in the next level
        nodeOffset += nodesPerLevel[i]; // add number of newly allocated child nodes to offset
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
