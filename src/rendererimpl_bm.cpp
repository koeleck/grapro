#include "rendererimpl_bm.h"

#include <cassert>
#include <algorithm>

#include "core/shader_manager.h"
#include "core/camera_manager.h"

#include "log/log.h"

#include "framework/vars.h"

#include "voxel.h"

/****************************************************************************/

RendererImplBM::RendererImplBM(core::TimerArray& timer_array)
  : RendererInterface{timer_array}
{

    // octree building
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag_bm.comp", GL_COMPUTE_SHADER);
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc_bm.comp", GL_COMPUTE_SHADER);
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

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
    unsigned int mem = totalNodes * 4 +
        static_cast<unsigned int>(vars.max_voxel_fragments * sizeof(VoxelStruct));
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
    LOG_INFO("max nodes: ", totalNodes, ", max fragments: ", vars.max_voxel_fragments, " (", mem, unit, ")");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, totalNodes * sizeof(OctreeNodeStruct), nullptr, GL_MAP_READ_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

/****************************************************************************/

RendererImplBM::~RendererImplBM() = default;

/****************************************************************************/

void RendererImplBM::genAtomicBuffer()
{
    GLuint initVal = 0;

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initVal, GL_STATIC_DRAW);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

/****************************************************************************/

void RendererImplBM::genVoxelBuffer()
{

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
}

/****************************************************************************/

void RendererImplBM::genOctreeNodeBuffer()
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

void RendererImplBM::createVoxelList(const bool debug_output)
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
    GLint loc = glGetUniformLocation(m_voxel_prog, "uNumVoxels");
    glUniform1i(loc, dim);

    // buffer
    genAtomicBuffer();
    genVoxelBuffer();

    // render
    renderGeometry(m_voxel_prog);

    // get number of voxel fragments
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    auto count = static_cast<GLuint *>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    m_numVoxelFrag = count[0];

    m_voxelize_timer->stop();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag);
        LOG_INFO("");
    }

}

/****************************************************************************/

void RendererImplBM::buildVoxelTree(const bool debug_output)
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

    createVoxelBBoxes(nodeOffset);

}

/****************************************************************************/

void RendererImplBM::render(const unsigned int tree_levels, const bool renderBBoxes,
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
