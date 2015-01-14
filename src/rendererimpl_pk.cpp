#include "rendererimpl_pk.h"

#include <cassert>
#include <algorithm>

#include "core/shader_manager.h"
#include "core/camera_manager.h"
#include "core/timer_array.h"

#include "log/log.h"

#include "framework/vars.h"

#include "voxel.h"

/****************************************************************************/

namespace
{

constexpr int FLAG_PROG_LOCAL_SIZE {64};
constexpr int ALLOC_PROG_LOCAL_SIZE {256};

} // anonymous namespace

/****************************************************************************/

RendererImplPK::RendererImplPK(core::TimerArray& timer_array)
  : RendererInterface{timer_array}
{

    // octree building
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag_pk.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(FLAG_PROG_LOCAL_SIZE));
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc_pk.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(ALLOC_PROG_LOCAL_SIZE));
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

    // voxel buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelStruct), nullptr, GL_MAP_READ_BIT);

    // node buffer
    auto max_num_nodes = 1u;
    auto tmp = 1u;
    for (auto i = 0u; i < vars.voxel_octree_levels; ++i) {
        tmp *= 8;
        max_num_nodes += tmp;
    }
    auto mem = max_num_nodes * 4 +
        static_cast<unsigned int>(vars.max_voxel_fragments * sizeof(VoxelStruct));
    std::string unit {"B"};
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
    LOG_INFO("max nodes: ", max_num_nodes, ", max fragments: ", vars.max_voxel_fragments, " (", mem, unit, ")");
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

RendererImplPK::~RendererImplPK() = default;

/****************************************************************************/

void RendererImplPK::createVoxelList(const bool debug_output)
{

    if (debug_output) {
        LOG_INFO("\n###########################\n"
                   "# Voxel Fragment Creation #\n"
                   "###########################");
    }

    m_voxelize_timer->start();

    auto* old_cam = core::res::cameras->getDefaultCam();
    core::res::cameras->makeDefault(m_voxelize_cam);

    const auto num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));

    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glViewport(0, 0, num_voxels, num_voxels);

    const auto loc = glGetUniformLocation(m_voxel_prog, "uNumVoxels");
    glProgramUniform1i(m_voxel_prog, loc, num_voxels);

    // reset counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    const auto zero = GLuint{};
    glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint),
            GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // setup
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, 0, sizeof(GLuint));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);

    renderGeometry(m_voxel_prog);

    // TODO move to shader
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &m_numVoxelFrag);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    m_voxelize_timer->stop();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);

    if (debug_output) {
        LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag, "/", vars.max_voxel_fragments);

        if (m_numVoxelFrag > vars.max_voxel_fragments) {
            LOG_WARNING("TOO MANY VOXEL FRAGMENTS!");
        }
    }

}

/****************************************************************************/

void RendererImplPK::buildVoxelTree(const bool debug_output)
{

    if (debug_output) {
        LOG_INFO("\n###########################\n"
                   "#     Octree Creation     #\n"
                   "###########################");
    }

    m_tree_timer->start();

    // calculate max invocations for compute shader to get all the voxel fragments
    const auto flag_num_workgroups = static_cast<unsigned int>((m_numVoxelFrag + FLAG_PROG_LOCAL_SIZE - 1)
                                                                / FLAG_PROG_LOCAL_SIZE);

    auto loc = GLint{};
    const auto flag_prog = m_octreeNodeFlag_prog;
    const auto alloc_prog = m_octreeNodeAlloc_prog;

    // uniforms
    loc = glGetUniformLocation(flag_prog, "uNumVoxelFrag");
    glProgramUniform1ui(flag_prog, loc, m_numVoxelFrag);
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "uTreeLevels");
    glProgramUniform1ui(flag_prog, loc, vars.voxel_octree_levels);

    const auto uMaxLevel = glGetUniformLocation(flag_prog, "uMaxLevel");
    const auto uStartNode = glGetUniformLocation(alloc_prog, "uStartNode");
    const auto uCount = glGetUniformLocation(alloc_prog, "uCount");

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, sizeof(GLuint), sizeof(GLuint));

    auto previously_allocated = 8u; // only root node was allocated
    auto numAllocated = 8u; // we're only allocating one block of 8 nodes, so yeah, 8;
    auto start_node = 0u;
    for (auto i = 0u; i < vars.voxel_octree_levels; ++i) {

        if (debug_output) { LOG_INFO("Starting with max level ", i); }

        /*
         *  flag nodes
         */
        if (i == 0) {
            // rootnode will always be subdivided, so don't run
            // the compute shader, but flag the first node
            // with glClearBufferSubData
            const auto flag = GLuint{0x80000000};
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(GLuint),
                GL_RED_INTEGER, GL_UNSIGNED_INT, &flag);

            // clear the remaining 7 nodes
            const auto zero = GLuint{};
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, sizeof(GLuint),
                7 * sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

            // set the allocation counter to 1, so we don't overwrite the root node
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
            const auto one = zero + 1;
            glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, sizeof(GLuint), sizeof(GLuint),
                GL_RED_INTEGER, GL_UNSIGNED_INT, &one);

        } else {
            glUseProgram(flag_prog);

            glProgramUniform1ui(flag_prog, uMaxLevel, i);

            glDispatchCompute(flag_num_workgroups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        if (i + 1 == vars.voxel_octree_levels) {
            // no more nodes required
            break;
        }

        /*
         *  allocate child nodes
         */
        glUseProgram(alloc_prog);

        glProgramUniform1ui(alloc_prog, uCount, previously_allocated);
        glProgramUniform1ui(alloc_prog, uStartNode, start_node);

        const auto alloc_workgroups = static_cast<GLuint>((previously_allocated + ALLOC_PROG_LOCAL_SIZE - 1)
                                                          / ALLOC_PROG_LOCAL_SIZE);
        glDispatchCompute(alloc_workgroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        // start_node points to the first node that was allocated in this iteration
        start_node += previously_allocated;

        /*
         *  How many child node were allocated?
         */
        auto counter_value = GLuint{};
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), sizeof(GLuint), &counter_value);
        counter_value *= 8;
        previously_allocated = counter_value - numAllocated;
        numAllocated = counter_value;

        if (debug_output) { LOG_INFO(" num allocated this iterator: ", previously_allocated); }
    }
    m_tree_timer->stop();

    if (debug_output) { LOG_INFO(":: Total Nodes created: ", numAllocated); }

    // TODO
    ///*
    // *  write information to leafs
    // */

    createVoxelBBoxes(numAllocated);

}

/****************************************************************************/

void RendererImplPK::render(const unsigned int tree_levels, const bool renderBBoxes,
                        const bool renderOctree, const bool debug_output)
{
    if (m_geometry.empty())
        return;

    if (m_rebuildTree) {
        createVoxelList(debug_output);
        buildVoxelTree(debug_output);
        m_rebuildTree = false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    renderGeometry(m_vertexpulling_prog);

    if (renderBBoxes)
        renderBoundingBoxes();
    if (renderOctree)
        renderVoxelBoundingBoxes();
}

/****************************************************************************/
