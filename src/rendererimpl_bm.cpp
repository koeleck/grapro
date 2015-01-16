#include "rendererimpl_bm.h"

#include <cassert>
#include <algorithm>

#include "core/shader_manager.h"
#include "core/camera_manager.h"
#include "core/timer_array.h"
#include "core/instance_manager.h"
#include "core/mesh_manager.h"

#include "log/log.h"

#include "framework/vars.h"

#include "voxel.h"

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

    core::res::shaders->registerShader("colorboxes_frag", "tree/colorboxes.frag",
            GL_FRAGMENT_SHADER);
    m_colorboxes_prog = core::res::shaders->registerProgram("colorboxes_prog",
            {"vertexpulling_vert", "colorboxes_frag"});

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

    core::res::shaders->registerShader("octreeLeafStoreComp", "tree/leafstore.comp", GL_COMPUTE_SHADER);
    m_octreeLeafStore_prog = core::res::shaders->registerProgram("octreeLeafStore_prog", {"octreeLeafStoreComp"});

    core::res::shaders->registerShader("octreeMipMapComp", "tree/mipmap.comp", GL_COMPUTE_SHADER);
    m_octreeMipMap_prog = core::res::shaders->registerProgram("octreeMipMap_prog", {"octreeMipMapComp"});
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

void RendererImplBM::resetBuffer(const gl::Buffer & buf, const int binding) const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);

    // fill with zeroes
    const auto zero = GLuint{};
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // bind to binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buf);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
    resetBuffer(m_voxelBuffer, core::bindings::VOXEL);

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

    // calculate max threads for flag shader to get all the voxel fragments
    const auto dataWidth = 1024u;
    const auto dataHeight = static_cast<unsigned int>((m_numVoxelFrag + dataWidth - 1) / dataWidth);
    const auto groupDimX = static_cast<unsigned int>(dataWidth / 8);
    const auto groupDimY = static_cast<unsigned int>((dataHeight + 7) / 8);
    const auto allocwidth = 64u;

    // octree buffer
    resetBuffer(m_octreeNodeBuffer, core::bindings::OCTREE);
    resetBuffer(m_octreeNodeColorBuffer, core::bindings::OCTREE_COLOR);

    // atomic counter (counts how many child node have to be allocated)
    resetAtomicBuffer();
    const auto zero = GLuint{}; // for resetting the atomic counter

    // uniforms
    const auto loc_u_numVoxelFrag = glGetUniformLocation(m_octreeNodeFlag_prog, "u_numVoxelFrag");
    const auto loc_u_voxelDim = glGetUniformLocation(m_octreeNodeFlag_prog, "u_voxelDim");
    const auto loc_u_maxLevel = glGetUniformLocation(m_octreeNodeFlag_prog, "u_maxLevel");

    const auto loc_u_numNodesThisLevel = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_numNodesThisLevel");
    const auto loc_u_nodeOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_nodeOffset");
    const auto loc_u_allocOffset = glGetUniformLocation(m_octreeNodeAlloc_prog, "u_allocOffset");

    const auto loc_u_numVoxelFrag_Store = glGetUniformLocation(m_octreeLeafStore_prog, "u_numVoxelFrag");
    const auto loc_u_voxelDim_Store = glGetUniformLocation(m_octreeLeafStore_prog, "u_voxelDim");
    const auto loc_u_treeLevels = glGetUniformLocation(m_octreeLeafStore_prog, "u_treeLevels");

    const auto loc_u_numVoxelFrag_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_numVoxelFrag");
    const auto loc_u_level = glGetUniformLocation(m_octreeMipMap_prog, "u_level");
    const auto loc_u_voxelDim_MipMap = glGetUniformLocation(m_octreeMipMap_prog, "u_voxelDim");

    const auto voxelDim = static_cast<unsigned int>(std::pow(2, m_treeLevels));

    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_numVoxelFrag, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeNodeFlag_prog, loc_u_voxelDim, voxelDim);

    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_numVoxelFrag_Store, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_voxelDim_Store, voxelDim);
    glProgramUniform1ui(m_octreeLeafStore_prog, loc_u_treeLevels, m_treeLevels);

    glProgramUniform1ui(m_octreeMipMap_prog, loc_u_numVoxelFrag_MipMap, m_numVoxelFrag);
    glProgramUniform1ui(m_octreeMipMap_prog, loc_u_voxelDim_MipMap, voxelDim);

    auto nodeOffset = 0u; // offset to first node of next level
    auto allocOffset = 1u; // offset to first free space for new nodes
    auto maxNodesPerLevel = std::vector<unsigned int>{1}; // number of nodes in each tree level; root level = 1

    for (auto i = 0u; i < m_treeLevels; ++i) {

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
        if (i == m_treeLevels - 1) {

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
        const auto allocGroupDim = (maxNodesPerLevel[i] + allocwidth - 1) / allocwidth;
        if (debug_output) {
            LOG_INFO("Dispatching NodeAlloc with ", allocGroupDim, "*1*1 groups with 64*1*1 threads each");
            LOG_INFO("--> ", allocGroupDim * 64, " threads");
        }
        glDispatchCompute(allocGroupDim, 1, 1);
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
        nodeOffset += maxNodesPerLevel[i];                      // add number of newly allocated child nodes to offset
        allocOffset += maxNodesToBeAllocated;

    }

    if (debug_output) {
        LOG_INFO("");
        LOG_INFO("Total Nodes created: ", nodeOffset);
        LOG_INFO("");
    }

    m_tree_timer->stop();

    m_mipmap_timer->start();

    /*
     *  write information to leafs
     */

    glUseProgram(m_octreeLeafStore_prog);

    // dispatch
    if (debug_output) {
        LOG_INFO("Dispatching LeafStore with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
        LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
    }
    glDispatchCompute(groupDimX, groupDimY, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /*
     *  mip map higher levels
     */

    auto i = m_treeLevels - 1;
    assert(m_treeLevels != 0);
    while (m_treeLevels != 1) {

        glUseProgram(m_octreeMipMap_prog);

        // uniforms
        glProgramUniform1ui(m_octreeMipMap_prog, loc_u_level, i);

        // dispatch
        if (debug_output) {
            LOG_INFO("Dispatching MipMap with ", groupDimX, "*", groupDimY, "*1 groups with 8*8*1 threads each");
            LOG_INFO("--> ", groupDimX * groupDimY * 64, " threads");
        }
        glDispatchCompute(groupDimX, groupDimY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        if (--i == 0u) {
            break;
        }

    }

    m_mipmap_timer->stop();

    createVoxelBBoxes(nodeOffset);

}

/****************************************************************************/

void RendererImplBM::render(const unsigned int treeLevels, const bool renderBBoxes,
                      const bool renderOctree, const bool debug_output)
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
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    // renderGeometry(m_vertexpulling_prog);
    //renderAmbientOcclusion();
    renderColorBoxes();

    if (renderBBoxes)
        renderBoundingBoxes();

    if (renderOctree)
        renderVoxelBoundingBoxes();
}

/****************************************************************************/

void RendererImplBM::initAmbientOcclusion()
{
    // shader
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("gbuffer_frag", "conetracing/gbuffer.frag", GL_FRAGMENT_SHADER);
    m_gbuffer_prog = core::res::shaders->registerProgram("gbuffer_prog", {"vertexpulling_vert", "gbuffer_frag"});

    core::res::shaders->registerShader("ssq_ao_vert", "conetracing/ssq_ao.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("ssq_ao_frag", "conetracing/ssq_ao.frag", GL_FRAGMENT_SHADER);
    m_ssq_ao_prog = core::res::shaders->registerProgram("ssq_ao_prog", {"ssq_ao_vert", "ssq_ao_frag"});

    // textures
    glBindTexture(GL_TEXTURE_2D, m_tex_position.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vars.screen_width, vars.screen_height, 0, GL_RGBA, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, m_tex_normal.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, vars.screen_width, vars.screen_height, 0, GL_RGBA, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, m_tex_color.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vars.screen_width, vars.screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, m_tex_depth.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, vars.screen_width, vars.screen_height, 0, GL_RGBA, GL_FLOAT, 0);

    // FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer_FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_position, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_tex_normal, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, m_tex_color, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, m_tex_depth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty framebuffer is not complete (WTF?!?)");
    }

    // screen space quad
    glBindVertexArray(m_vao_ssq.get());
    float ssq[] = { -1.f, -1.f, 0.f,
                    1.f, -1.f, 0.f,
                    1.f, 1.f, 0.f,
                    1.f, 1.f, 0.f,
                    -1.f, 1.f, 0.f,
                    -1.f, -1.f, 0.f };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_ssq.get());
    glBufferData(GL_ARRAY_BUFFER, sizeof(ssq), ssq, GL_STATIC_DRAW);
    glVertexAttribPointer(m_v_ssq, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(m_v_ssq);
}

/****************************************************************************/

void RendererImplBM::renderAmbientOcclusion() const
{

    /*
     *   First step: fill "gbuffer" with camera <-> scene intersection
     *  positions.
     */

    // bind gbuffer texture
    glBindTexture(GL_TEXTURE_2D, m_tex_position.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_normal.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_color.get());
    glBindTexture(GL_TEXTURE_2D, m_tex_depth.get());

    // FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer_FBO);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    GLenum DrawBuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, DrawBuffers);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderGeometry(m_gbuffer_prog);

    // unbind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_tex_depth.get());

    auto loc = glGetUniformLocation(m_ssq_ao_prog, "u_pos");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_normal");
    glUniform1i(loc, 1);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_color");
    glUniform1i(loc, 2);
    loc = glGetUniformLocation(m_ssq_ao_prog, "u_depth");
    glUniform1i(loc, 3);

    glBindVertexArray(m_vao_ssq.get());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    

    // TODO: FIX THIS
    // debug: check if the gbuffer textures are filled
    /*glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer_FBO);
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
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    glBlitFramebuffer(0, 0, vars.screen_width, vars.screen_height,
                      vars.screen_width/2, vars.screen_height/2, vars.screen_width, vars.screen_height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);*/
}

/****************************************************************************/

void RendererImplBM::renderColorBoxes() const
{
    // uniforms
    const auto dim = static_cast<int>(std::pow(2.0, m_treeLevels - 1));
    auto loc = glGetUniformLocation(m_colorboxes_prog, "u_bboxMin");
    glProgramUniform3f(m_colorboxes_prog, loc, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_bboxMax");
    glProgramUniform3f(m_colorboxes_prog, loc, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_voxelDim");
    glProgramUniform1ui(m_colorboxes_prog, loc, dim);
    loc = glGetUniformLocation(m_colorboxes_prog, "u_treeLevels");
    glProgramUniform1ui(m_colorboxes_prog, loc, m_treeLevels);

    renderGeometry(m_colorboxes_prog);
}

/****************************************************************************/
