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

RendererImplPK::RendererImplPK(core::TimerArray& timer_array, unsigned int treeLevels)
  : RendererInterface{timer_array, treeLevels}
{

    initShaders();

    // voxel buffer
    auto sizeOfVoxels = vars.max_voxel_fragments * sizeof(VoxelStruct);
    recreateBuffer(m_voxelBuffer, sizeOfVoxels);

    // node buffer
    auto max_num_nodes = calculateMaxNodes();

    auto mem = max_num_nodes * 4 + sizeOfVoxels;
    auto unit = "B";
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

    recreateBuffer(m_octreeNodeBuffer, max_num_nodes * sizeof(OctreeNodeStruct));
    //glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    recreateBuffer(m_octreeNodeColorBuffer, max_num_nodes * sizeof(OctreeNodeColorStruct));

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

void RendererImplPK::initShaders()
{
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag_pk.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(FLAG_PROG_LOCAL_SIZE));
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc_pk.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(ALLOC_PROG_LOCAL_SIZE));
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});
}

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

    const auto num_voxels = static_cast<int>(std::pow(2.0, m_treeLevels - 1));

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
    auto calculateDataWidth = [&](unsigned int num, unsigned width) {
        return (num + width - 1) / width;
    };
    const auto localWidth = 64u;
    const auto fragWidth = calculateDataWidth(m_numVoxelFrag, localWidth);

    auto loc = GLint{};
    const auto flag_prog = m_octreeNodeFlag_prog;
    const auto alloc_prog = m_octreeNodeAlloc_prog;

    // uniforms
    loc = glGetUniformLocation(flag_prog, "uNumVoxelFrag");
    glProgramUniform1ui(flag_prog, loc, m_numVoxelFrag);
    loc = glGetUniformLocation(m_octreeNodeFlag_prog, "uTreeLevels");
    glProgramUniform1ui(flag_prog, loc, m_treeLevels);

    const auto uMaxLevel = glGetUniformLocation(flag_prog, "uMaxLevel");
    const auto uStartNode = glGetUniformLocation(alloc_prog, "uStartNode");
    const auto uCount = glGetUniformLocation(alloc_prog, "uCount");

    const auto loc_u_numVoxelFrag_Store = glGetUniformLocation(m_octreeLeafStore_prog, "u_numVoxelFrag");
    const auto loc_u_voxelDim_Store = glGetUniformLocation(m_octreeLeafStore_prog, "u_voxelDim");
    const auto loc_u_treeLevels = glGetUniformLocation(m_octreeLeafStore_prog, "u_treeLevels");

    const auto loc_u_numVoxelFrag_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_numVoxelFrag");
    const auto loc_u_level = glGetUniformLocation(m_octreeMipMap_prog, "u_level");
    const auto loc_u_voxelDim_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_voxelDim");

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));

    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_numVoxelFrag_Store, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_voxelDim_Store, voxelDim);
    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_treeLevels, m_treeLevels);

    glProgramUniform1ui(m_octreeMipMap_prog, loc_u_numVoxelFrag_MipMap, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeMipMap_prog, loc_u_voxelDim_MipMap, voxelDim);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE_COLOR, m_octreeNodeColorBuffer);
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, sizeof(GLuint), sizeof(GLuint));

    auto previously_allocated = 8u; // only root node was allocated
    auto numAllocated = 8u; // we're only allocating one block of 8 nodes, so yeah, 8;
    auto start_node = 0u;
    for (auto i = 0u; i < m_treeLevels; ++i) {

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

            glDispatchComputeGroupSizeARB(fragWidth, 1, 1,
                                          FLAG_PROG_LOCAL_SIZE, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        if (i + 1 == m_treeLevels) {
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
        glDispatchComputeGroupSizeARB(alloc_workgroups, 1, 1,
                                      ALLOC_PROG_LOCAL_SIZE, 1, 1);
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

    if (debug_output) { LOG_INFO(":: Total Nodes created: ", numAllocated); }

    m_tree_timer->stop();

    m_mipmap_timer->start();

    /*
     *  write information to leafs
     */

    glUseProgram(m_octreeLeafStore_prog);

    // dispatch
    glDispatchComputeGroupSizeARB(fragWidth, 1, 1,
                                  localWidth, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /*
     *  mip map higher levels
     */

    auto i = m_treeLevels - 1;
    assert(m_treeLevels != 0);
    while (i > 0u) {

        glUseProgram(m_octreeMipMap_prog);

        // uniforms
        glProgramUniform1ui(m_octreeMipMap_prog, loc_u_level, i);

        // dispatch
        const auto groupWidth = calculateDataWidth(/*maxNodesPerLevel[i]*/m_numVoxelFrag, FLAG_PROG_LOCAL_SIZE);
        glDispatchComputeGroupSizeARB(groupWidth, 1, 1,
                                      FLAG_PROG_LOCAL_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        --i;

    }

    m_mipmap_timer->stop();

    createVoxelBBoxes(numAllocated);

}

/****************************************************************************/

void RendererImplPK::render(const unsigned int treeLevels, const unsigned int voxelColorLevel,
                            const bool renderBBoxes, const bool renderOctree,
                            const bool renderVoxColors, const bool debug_output)
{
    if (m_geometry.empty())
        return;

    if (treeLevels != m_treeLevels) {
        m_treeLevels = treeLevels;
        m_rebuildTree = true;

        auto totalNodes = calculateMaxNodes();
        recreateBuffer(m_octreeNodeBuffer, totalNodes * sizeof(OctreeNodeStruct));
        recreateBuffer(m_octreeNodeColorBuffer, totalNodes * sizeof(OctreeNodeColorStruct));
        resizeFBO();
    }

    if (m_rebuildTree) {
        createVoxelList(debug_output);
        buildVoxelTree(debug_output);
        m_rebuildTree = false;

        gl::printInfo();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    if (renderVoxColors) {
        if (voxelColorLevel != m_voxelColorLevel) {
            m_voxelColorLevel = voxelColorLevel;
        }
        renderVoxelColors();
    } else {
        renderGeometry(m_vertexpulling_prog);
    }

    if (renderBBoxes)
        renderBoundingBoxes();

    if (renderOctree)
        renderVoxelBoundingBoxes();
}

/****************************************************************************/
