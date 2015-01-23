#include "rendererimpl_bm.h"

#include <cassert>
#include <algorithm>

#include "core/shader_manager.h"
#include "core/camera_manager.h"
#include "core/timer_array.h"
#include "core/instance_manager.h"
#include "core/mesh_manager.h"
#include "core/mesh.h"
#include "core/camera_manager.h"
#include "core/material_manager.h"
#include "core/shader_interface.h"
#include "core/texture_manager.h"
#include "core/light_manager.h"
#include "core/texture.h"
#include "log/log.h"

#include "framework/vars.h"

#include "voxel.h"

/****************************************************************************/

namespace
{
constexpr unsigned int FLAG_PROG_LOCAL_SIZE {64u};
constexpr int ALLOC_PROG_LOCAL_SIZE = 256;
typedef struct
{
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
} DrawElementsIndirectCommand;
} // anonymous namespace

/****************************************************************************/

RendererImplBM::RendererImplBM(core::TimerArray& timer_array, unsigned int treeLevels)
  : RendererInterface{timer_array, treeLevels}
{

    initShaders();

    initAmbientOcclusion();


    // allocate voxelBuffer
    auto sizeOfVoxels = vars.max_voxel_fragments * sizeof(VoxelStruct);
    recreateBuffer(m_voxelBuffer, sizeOfVoxels);

    // allocate octreeBuffer
    auto totalNodes = calculateMaxNodes();

    auto mem = totalNodes * 4 + sizeOfVoxels;
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
    LOG_INFO("max nodes: ", totalNodes, ", max fragments: ", vars.max_voxel_fragments, " (", mem, unit, ")");

    recreateBuffer(m_octreeNodeBuffer, totalNodes * sizeof(OctreeNodeStruct));
    recreateBuffer(m_octreeNodeColorBuffer, totalNodes * sizeof(OctreeNodeColorStruct));
    recreateBuffer(m_brickBuffer, totalNodes * sizeof(BrickStruct));

}

/****************************************************************************/

RendererImplBM::~RendererImplBM() = default;

/****************************************************************************/

void RendererImplBM::initShaders()
{
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/nodeflag_bm.comp", GL_COMPUTE_SHADER);
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocComp", "tree/nodealloc_bm.comp", GL_COMPUTE_SHADER);
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeMipMapComp", "tree/mipmap.comp", GL_COMPUTE_SHADER);
    m_octreeMipMap_prog = core::res::shaders->registerProgram("octreeMipMap_prog", {"octreeMipMapComp"});

    core::res::shaders->registerShader("ssq_ao_vert", "conetracing/ssq_ao.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("indirectDiffuse_frag", "conetracing/indirect_diffuse.frag", GL_FRAGMENT_SHADER);
    m_indirectDiffuse_prog = core::res::shaders->registerProgram("indirectDiffuse_prog", {"ssq_ao_vert", "indirectDiffuse_frag"});

    core::res::shaders->registerShader("indirectSpecular_frag", "conetracing/indirect_specular.frag", GL_FRAGMENT_SHADER);
    m_indirectSpecular_prog = core::res::shaders->registerProgram("indirectSpecular_prog", {"ssq_ao_vert", "indirectSpecular_frag"});

    // shadows
    core::res::shaders->registerShader("shadow_vert", "basic/shadow.vert",
    GL_VERTEX_SHADER);
    core::res::shaders->registerShader("shadow_geom", "basic/shadow.geom",
    GL_GEOMETRY_SHADER, "NUM_SHADOWMAPS " + std::to_string(std::max(1, core::res::lights->getNumShadowMapsUsed())));
    core::res::shaders->registerShader("shadow_cube_geom", "basic/shadow_cube.geom",
    GL_GEOMETRY_SHADER, "NUM_SHADOWCUBEMAPS " + std::to_string(std::max(1, core::res::lights->getNumShadowCubeMapsUsed())));
    core::res::shaders->registerShader("depth_only_frag", "basic/depth_only.frag",
    GL_FRAGMENT_SHADER);
    m_2d_shadow_prog = core::res::shaders->registerProgram("shadow2d_prog",
    {"shadow_vert", "shadow_geom", "depth_only_frag"});
    m_cube_shadow_prog = core::res::shaders->registerProgram("shadow_cube_prog",
    {"shadow_vert", "shadow_cube_geom", "depth_only_frag"});
}

/****************************************************************************/

void RendererImplBM::resetAtomicBuffer() const
{
    const auto zero = GLuint{};

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_STATIC_DRAW);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
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

    const auto dim = static_cast<int>(std::pow(2.0, m_treeLevels - 1));

    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glViewport(0, 0, dim, dim);

    glUseProgram(m_voxel_prog);

    // uniforms
    const auto loc = glGetUniformLocation(m_voxel_prog, "uNumVoxels");
    glUniform1i(loc, dim);

    // buffer
    resetAtomicBuffer();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    // render
    renderGeometry(m_voxel_prog);

    // get number of voxel fragments
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    const auto count = static_cast<GLuint *>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
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

    // calculate max threads for shaders
    auto calculateDataWidth = [&](unsigned int num, unsigned width) {
        return (num + width - 1) / width;
    };
    const auto fragWidth = calculateDataWidth(m_numVoxelFrag, FLAG_PROG_LOCAL_SIZE);
    auto groupWidth = 0u;

    // octree buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);

    // atomic counter (counts how many child node have to be allocated)
    resetAtomicBuffer();
    const auto zero = GLuint{}; // for resetting the atomic counter

    // uniforms
    const auto loc_u_numVoxelFrag = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
    const auto loc_u_voxelDim = glGetUniformLocation(m_octreeNodeFlag_prog, "u_voxelDim");
    const auto loc_u_maxLevel = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");
    const auto loc_u_isLeaf = glGetUniformLocation(m_octreeNodeFlag_prog, "u_isLeaf");

    const auto loc_u_numNodesThisLevel = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_numNodesThisLevel");
    const auto loc_u_nodeOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_nodeOffset");
    const auto loc_u_allocOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_allocOffset");

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));

    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_numVoxelFrag, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_voxelDim, voxelDim);

    auto nodeOffset = 0u; // offset to first node of next level
    auto allocOffset = 1u; // offset to first free space for new nodes
    auto maxNodesPerLevel = std::vector<unsigned int>{1}; // number of nodes in each tree level; root level = 1


    /*
     *  flag root node
     */

    glUseProgram(m_octreeNodeFlag_prog);

    // uniforms
    bool isLeaf = (0 == m_treeLevels - 1);
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_maxLevel, 0);
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_isLeaf, isLeaf ? 1 : 0);
    if (isLeaf) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE_COLOR, m_octreeNodeColorBuffer);
    }

    // dispatch
    if (debug_output) {
        LOG_INFO("Dispatching NodeFlag with 1*1*1 groups with 64*1*1 threads each");
        LOG_INFO("--> ", FLAG_PROG_LOCAL_SIZE, " threads");
    }
    glDispatchComputeGroupSizeARB(1, 1, 1,
                                  FLAG_PROG_LOCAL_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    for (auto i = 1u; i < m_treeLevels; ++i) {

        if (debug_output) {
            LOG_INFO("");
            LOG_INFO("Starting with max level ", i);
        }

        /*
         *  allocate child nodes
         */

        glUseProgram(m_octreeNodeAlloc_prog);

        // uniforms
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_numNodesThisLevel, maxNodesPerLevel[i - 1]);
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_nodeOffset, nodeOffset);
        glProgramUniform1ui(m_octreeNodeAlloc_prog, loc_u_allocOffset, allocOffset);

        // dispatch
        groupWidth = calculateDataWidth(maxNodesPerLevel[i - 1], FLAG_PROG_LOCAL_SIZE);
        if (debug_output) {
            LOG_INFO("Dispatching NodeAlloc with ", groupWidth, "*1*1 groups with 64*1*1 threads each");
            LOG_INFO("--> ", groupWidth * FLAG_PROG_LOCAL_SIZE, " threads");
        }
        glDispatchComputeGroupSizeARB(groupWidth, 1, 1,
                                      FLAG_PROG_LOCAL_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        /*
         *  get how many nodes have to be allocated at most
         */

        GLuint actualNodesAllocated{};
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &actualNodesAllocated);
        glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); //reset counter to zero
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        if (debug_output) { LOG_INFO(actualNodesAllocated, " nodes have been allocated"); }
        const auto maxNodesToBeAllocated = actualNodesAllocated * 8;

        /*
         *  update offsets
         */

        maxNodesPerLevel.emplace_back(maxNodesToBeAllocated);   // maxNodesToBeAllocated is the number of threads
                                                                // we want to launch in the next level
        nodeOffset += maxNodesPerLevel[i - 1];                      // add number of newly allocated child nodes to offset
        allocOffset += maxNodesToBeAllocated;

        /*
         *  flag nodes
         */

        glUseProgram(m_octreeNodeFlag_prog);

        // uniforms
        isLeaf = (i == m_treeLevels - 1);
        glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_maxLevel, i);
        glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_isLeaf, isLeaf ? 1 : 0);
        if (isLeaf) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE_COLOR, m_octreeNodeColorBuffer);
        }

        // dispatch
        if (debug_output) {
            LOG_INFO("Dispatching NodeFlag with ", fragWidth, "*1*1 groups with 64*1*1 threads each");
            LOG_INFO("--> ", fragWidth * FLAG_PROG_LOCAL_SIZE, " threads");
        }
        glDispatchComputeGroupSizeARB(fragWidth, 1, 1,
                                      FLAG_PROG_LOCAL_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    }

    m_tree_timer->stop();

    const auto totalNodesCreated = nodeOffset + maxNodesPerLevel.back();

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("Total Nodes created: ", totalNodesCreated);
        LOG_INFO("");
    }

    m_mipmap_timer->start();

    /*
     *  mip map to higher levels
     */

    glUseProgram(m_octreeMipMap_prog);

    // uniforms
    const auto loc_nodeOffset_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_nodeOffset");
    const auto loc_nodesThisLevel_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_nodesThisLevel");

    for (auto i = static_cast<int>(m_treeLevels) - 2; i >= 0; --i) {

        nodeOffset -= maxNodesPerLevel[i];
        if (debug_output) {
            LOG_INFO("");
            LOG_INFO("nodeOffset: ", nodeOffset, ", u_nodesThisLevel: ", maxNodesPerLevel[i]);
        }

        // uniforms
        glUniform1ui(loc_nodeOffset_MipMap, nodeOffset);
        glUniform1ui(loc_nodesThisLevel_MipMap, maxNodesPerLevel[i]);

        // dispatch
        groupWidth = calculateDataWidth(maxNodesPerLevel[i], FLAG_PROG_LOCAL_SIZE);
        if (debug_output) {
            LOG_INFO("Dispatching MipMap with ", groupWidth, "*1*1 groups with 64*1*1 threads each");
            LOG_INFO("--> ", groupWidth * FLAG_PROG_LOCAL_SIZE, " threads");
        }
        glDispatchComputeGroupSizeARB(groupWidth, 1, 1,
                                      FLAG_PROG_LOCAL_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    }

    m_mipmap_timer->stop();

    createVoxelBBoxes(totalNodesCreated);

}

/****************************************************************************/

void RendererImplBM::render(const unsigned int treeLevels, const bool renderBBoxes,
                            const bool renderOctree, const bool renderVoxColors,
                            const bool debug_output)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();
    core::res::lights->bind();

    if (treeLevels != m_treeLevels) {

        m_treeLevels = treeLevels;
        m_rebuildTree = true;

        auto totalNodes = calculateMaxNodes();
        recreateBuffer(m_octreeNodeBuffer, totalNodes * sizeof(OctreeNodeStruct));
        recreateBuffer(m_octreeNodeColorBuffer, totalNodes * sizeof(OctreeNodeColorStruct));
        resizeFBO();

        // zero out buffers! // TO DO: get rid of this and do not get color artifacts
        const auto zero = GLuint{};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        const auto zeroVec = glm::vec4{0};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeColorBuffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &zeroVec);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    } else if (m_rebuildTree) {

        // zero out buffers!
        const auto zero = GLuint{};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeColorBuffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    }

    if (m_rebuildTree) {
        // only render shadowmaps once
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        renderShadowmaps();

        createVoxelList(debug_output);
        buildVoxelTree(debug_output);
        m_rebuildTree = false;
        gl::printInfo();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    if (m_coneTracing) {
        coneTracing();
    } if (m_renderAO) {
        renderAmbientOcclusion();
    } else if (m_renderIndirectDiffuse) {
        renderIndirectDiffuseLighting();
    } else if (m_renderIndirectSpecular) {
        renderIndirectSpecularLighting();
    } else if (renderVoxColors) {
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

void RendererImplBM::initAmbientOcclusion()
{
    core::res::shaders->registerShader("ssq_ao_frag", "conetracing/ssq_ao.frag", GL_FRAGMENT_SHADER);
    m_ssq_ao_prog = core::res::shaders->registerProgram("ssq_ao_prog", {"ssq_ao_vert", "ssq_ao_frag"});
}

/****************************************************************************/

void RendererImplBM::renderAmbientOcclusion() const
{

    /*
     *   First step: fill "gbuffer" with camera <-> scene intersection
     *  positions.
     */

    renderToGBuffer();

    /*
     *  Render screen space quad
     */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_ssq_ao_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_position.get());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_tex_normal.get());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_tex_color.get());

    auto loc = glGetUniformLocation(m_ssq_ao_prog, "u_pos");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_normal");
    glUniform1i(loc, 1);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_color");
    glUniform1i(loc, 2);

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels - 1));
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_voxelDim");
    glUniform1ui(loc, voxelDim);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_bboxMin");
    glUniform3f(loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_bboxMax");
    glUniform3f(loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_screenwidth");
    glUniform1ui(loc, vars.screen_width);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_screenheight");
    glUniform1ui(loc, vars.screen_height);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_treeLevels");
    glUniform1ui(loc, m_treeLevels);

    loc = glGetUniformLocation(m_ssq_ao_prog, "u_coneGridSize");
    glUniform1ui(loc, m_ao_num_cones);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_numSteps");
    glUniform1ui(loc, m_ao_max_samples);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_weight");
    glUniform1ui(loc, m_ao_weight);


    glBindVertexArray(m_vao_ssq);
    glDrawArrays(GL_TRIANGLES, 0, 6);


    // TODO: FIX THIS
    // debug: check if the gbuffer textures are filled
    /*
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer_FBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, vars.screen_width, vars.screen_height,
                      0, 0, vars.screen_width/2, vars.screen_height/2,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, vars.screen_width, vars.screen_height,
                      0, vars.screen_height/2, vars.screen_width/2, vars.screen_height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glReadBuffer(GL_COLOR_ATTACHMENT2);
    glBlitFramebuffer(0, 0, vars.screen_width, vars.screen_height,
                      vars.screen_width/2, 0, vars.screen_width, vars.screen_height/2,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    */
}

/****************************************************************************/

void RendererImplBM::renderShadowmaps()
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    GLuint prog;
    if (core::res::lights->getNumShadowMapsUsed() > 0) {
        prog = m_2d_shadow_prog;
        core::res::lights->setupForShadowMapRendering();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderGeometry(prog);
    }
    if (core::res::lights->getNumShadowCubeMapsUsed() > 0) {
        prog = m_cube_shadow_prog;
        core::res::lights->setupForShadowCubeMapRendering();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderGeometry(prog);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
}

/****************************************************************************/
