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
#include "core/texture_manager.h"
#include "core/light_manager.h"
#include "core/texture.h"
#include "log/log.h"
#include "framework/vars.h"
#include "voxel.h"

/****************************************************************************/

namespace
{

constexpr int FLAG_PROG_LOCAL_SIZE = 256;
constexpr int ALLOC_PROG_LOCAL_SIZE = 256;
constexpr int INIT_PROG_LOCAL_SIZE = 256;
constexpr int INJECT_PROG_LOCAL_SIZE = 256;
constexpr int TO_NEIGHBORS_PROG_LOCAL_SIZE = 256;
constexpr int MIPMAP_PROG_LOCAL_SIZE = 128;

typedef struct
{
    GLuint  count;
    GLuint  instanceCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
} DrawElementsIndirectCommand;

} // anonymous namespace

/****************************************************************************/

struct Renderer::DrawCmd
{
    DrawCmd(const GLenum mode_, const GLenum type_, const GLvoid* const indirect_,
            const GLsizei drawcount_, const GLsizei stride_)
      : textures{0,},
        mode{mode_},
        type{type_},
        indirect{indirect_},
        drawcount{drawcount_},
        stride{stride_}
    {
    }

    GLuint                  textures[core::bindings::NUM_TEXT_UNITS];
    GLenum                  mode;
    GLenum                  type;
    const GLvoid*           indirect;
    GLsizei                 drawcount;
    GLsizei                 stride;
    core::AABB              aabb;
};

/****************************************************************************/

Renderer::Renderer(const int width, const int height, core::TimerArray& timer_array)
  : m_gbuffer{width, height},
    m_numVoxelFrag{0u},
    m_rebuildTree{true},
    m_timers(timer_array)
{

    initBBoxStuff();

    // programmable vertex pulling
    core::res::shaders->registerShader("vertexpulling_vert", "basic/vertexpulling.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("vertexpulling_frag", "basic/vertexpulling.frag",
            GL_FRAGMENT_SHADER);
    m_vertexpulling_prog = core::res::shaders->registerProgram("vertexpulling_prog",
            {"vertexpulling_vert", "vertexpulling_frag"});

    // shadows
    core::res::shaders->registerShader("shadow_vert", "basic/shadow.vert",
            GL_VERTEX_SHADER);
    core::res::shaders->registerShader("shadow_geom", "basic/shadow.geom",
            GL_GEOMETRY_SHADER,
            "NUM_SHADOWMAPS " + std::to_string(std::max(1, core::res::lights->getNumShadowMapsUsed())) +
            ", BIAS " + std::to_string(vars.light_bias));
    core::res::shaders->registerShader("shadow_cube_geom", "basic/shadow_cube.geom",
            GL_GEOMETRY_SHADER,
            "NUM_SHADOWCUBEMAPS " + std::to_string(std::max(1, core::res::lights->getNumShadowCubeMapsUsed())) +
            ", BIAS " + std::to_string(vars.light_bias));
    core::res::shaders->registerShader("depth_only_frag", "basic/depth_only.frag",
            GL_FRAGMENT_SHADER);
    m_2d_shadow_prog = core::res::shaders->registerProgram("shadow2d_prog",
            {"shadow_vert", "shadow_geom", "depth_only_frag"});
    m_cube_shadow_prog = core::res::shaders->registerProgram("shadow_cube_prog",
            {"shadow_vert", "shadow_cube_geom", "depth_only_frag"});

    core::res::shaders->registerShader("ssq_vert", "basic/ssq.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("direct_lighting_frag", "basic/direct_lighting.frag",
            GL_FRAGMENT_SHADER);
    m_direct_lighting_prog = core::res::shaders->registerProgram("direct_lighting_prog",
            {"ssq_vert", "direct_lighting_frag"});

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
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelFragmentStruct), nullptr, GL_MAP_READ_BIT);

    // node buffer
    int max_num_nodes = 1;
    int tmp = 1;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {
        tmp *= 8;
        max_num_nodes += tmp;
    }
    max_num_nodes = std::min(max_num_nodes, static_cast<int>(vars.max_voxel_nodes));
    vars.max_voxel_nodes = static_cast<unsigned int>(max_num_nodes);

    // brick texture
    GLint max_3d_tex_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_tex_size);
    int remaining = static_cast<int>(vars.max_voxel_nodes);
    int brick_num_x = std::min(remaining, max_3d_tex_size / 3);
    remaining = (remaining + brick_num_x - 1) / brick_num_x;
    int brick_num_y = std::min(remaining, max_3d_tex_size / 3);
    remaining = (remaining + brick_num_y - 1) / brick_num_y;
    int brick_num_z = std::min(remaining, max_3d_tex_size / 3);
    if ((brick_num_x * brick_num_y * brick_num_z) < static_cast<int>(vars.max_voxel_nodes)) {
        LOG_ERROR("brick texture can't store all nodes!");
        abort();
    }
    constexpr int BRICK_TEX_ELEMENT_SIZE = 2 * 4;
    m_brick_texture_size.x = brick_num_x * 3;
    m_brick_texture_size.y = brick_num_y * 3;
    m_brick_texture_size.z = brick_num_z * 3;

    glBindTexture(GL_TEXTURE_3D, m_brick_texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F,
            m_brick_texture_size.x,
            m_brick_texture_size.y,
            m_brick_texture_size.z);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    LOG_INFO("Brick texture: (", m_brick_texture_size.x, ", ",
            m_brick_texture_size.y, ", ",
            m_brick_texture_size.z, ")");


    int mem = max_num_nodes * (4 + static_cast<int>(sizeof(VoxelNodeInfo))) +
        (m_brick_texture_size.x * m_brick_texture_size.y * m_brick_texture_size.z * BRICK_TEX_ELEMENT_SIZE) +
        static_cast<int>(vars.max_voxel_fragments * sizeof(VoxelFragmentStruct));
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
    LOG_INFO("max nodes: ", max_num_nodes, ", max fragments: ", vars.max_voxel_fragments, " (", mem, unit, ")");

    // node buffer:
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
            static_cast<GLuint>(max_num_nodes) * sizeof(OctreeNodeStruct), nullptr, GL_MAP_READ_BIT);

    // node/leaf info buffer:
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeInfoBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
            static_cast<GLuint>(max_num_nodes) * sizeof(VoxelNodeInfo), nullptr, GL_MAP_READ_BIT);

    // Atomic counter
    // The first GLuint is for voxel fragments
    // The second GLuint is for nodes
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, 2 * sizeof(GLuint), nullptr, GL_MAP_READ_BIT);

    // FBO for voxelization
    int num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));
    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, num_voxels);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, num_voxels);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty framebuffer is not complete (WTF?!?)");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // shaders:
    const std::string bricks_def = "NUM_BRICKS_X " + std::to_string(brick_num_x) + ", "
            "NUM_BRICKS_Y " + std::to_string(brick_num_y) + ", "
            "NUM_BRICKS_Z " + std::to_string(brick_num_z);
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
            "LOCAL_SIZE " + std::to_string(ALLOC_PROG_LOCAL_SIZE) + ", " +
            bricks_def);
    m_octreeNodeAlloc_prog = core::res::shaders->registerProgram("octreeNodeAlloc_prog", {"octreeNodeAllocComp"});

    core::res::shaders->registerShader("octreeNodeInitComp", "tree/nodeinit.comp",
            GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(INIT_PROG_LOCAL_SIZE) + ", " +
            bricks_def);
    m_octreeNodeInit_prog = core::res::shaders->registerProgram("octreeNodeInit_prog",
            {"octreeNodeInitComp"});

    core::res::shaders->registerShader("octreeInjectLightingComp", "tree/inject_direct_lighting.comp",
            GL_COMPUTE_SHADER, "LOCAL_SIZE " + std::to_string(INJECT_PROG_LOCAL_SIZE) + ", " +
            bricks_def);
    m_inject_lighting_prog = core::res::shaders->registerProgram("octreeInjectLighting",
            {"octreeInjectLightingComp"});

    core::res::shaders->registerShader("to_neighbors_comp", "tree/to_neighbors.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(TO_NEIGHBORS_PROG_LOCAL_SIZE) + ", " +
            bricks_def);
    m_dist_to_neighbors_prog = core::res::shaders->registerProgram("to_neighbors_prog",
            {"to_neighbors_comp"});

    core::res::shaders->registerShader("mipmap_comp", "tree/mipmap.comp", GL_COMPUTE_SHADER,
            "LOCAL_SIZE " + std::to_string(MIPMAP_PROG_LOCAL_SIZE) + ", " +
            bricks_def);
    m_mipmap_prog = core::res::shaders->registerProgram("mipmap_prog",
            {"mipmap_comp"});

    core::res::shaders->registerShader("conetracing_frag", "conetracing/conetracing.frag",
            GL_FRAGMENT_SHADER, bricks_def);
    m_conetracing_prog = core::res::shaders->registerProgram("conetracing_prog",
            {"ssq_vert", "conetracing_frag"});

    // debug render
    core::res::shaders->registerShader("ssq_frag", "basic/ssq.frag", GL_FRAGMENT_SHADER);
    m_debug_tex_prog = core::res::shaders->registerProgram("ssq_prog", {"ssq_vert", "ssq_frag"});

    core::res::shaders->registerShader("octreeDebugBBox_vert", "tree/bbox.vert", GL_VERTEX_SHADER,
            bricks_def);
    core::res::shaders->registerShader("octreeDebugBBox_frag", "tree/bbox.frag", GL_FRAGMENT_SHADER,
            bricks_def);
    m_voxel_bbox_prog = core::res::shaders->registerProgram("octreeDebugBBox_prog",
            {"octreeDebugBBox_vert", "octreeDebugBBox_frag"});

    core::res::shaders->registerShader("diffuse_frag", "basic/diffuse.frag", GL_FRAGMENT_SHADER);
    m_diffuse_prog = core::res::shaders->registerProgram("diffuse_prog",
            {"vertexpulling_vert", "diffuse_frag"});

}

/****************************************************************************/

Renderer::~Renderer() = default;

/****************************************************************************/

void Renderer::resize(const int width, const int height)
{
    m_gbuffer = GBuffer(width, height);
}

/****************************************************************************/

void Renderer::setGeometry(std::vector<const core::Instance*> geometry)
{
    m_geometry = std::move(geometry);

    // sort geometry by material first and type second
    std::sort(m_geometry.begin(), m_geometry.end(),
            [] (const core::Instance* g0, const core::Instance* g1) -> bool
            {
                const auto* mat0 = g0->getMaterial();
                const auto* mat1 = g1->getMaterial();
                if (mat0->isCompatible(*mat1) == false)
                    return g0 < g1;
                const auto* mesh0 = g0->getMesh();
                const auto* mesh1 = g1->getMesh();
                return mesh0->type() < mesh1->type();
            });

    m_scene_bbox = core::AABB();
    m_drawlist.clear();
    m_drawlist.reserve(m_geometry.size());

    std::vector<DrawElementsIndirectCommand> indirectCmds;
    indirectCmds.reserve(m_geometry.size());

    std::size_t indirect = 0;
    GLenum current_type = 0;
    const core::Material* current_mat = nullptr;
    for (const auto* g : m_geometry) {
        m_scene_bbox.expandBy(g->getBoundingBox());

        const auto* mesh = g->getMesh();
        const auto* mat = g->getMaterial();

        //instanceIDs.emplace_back(g->getIndex());
        indirectCmds.emplace_back();
        auto& indirectCmd = indirectCmds.back();
        indirectCmd.count = static_cast<GLuint>(mesh->count());
        indirectCmd.instanceCount = 1;
        indirectCmd.firstIndex = mesh->firstIndex();
        indirectCmd.baseVertex = 0;
        indirectCmd.baseInstance = g->getIndex();


        if (mesh->type() != current_type || current_mat != mat) {
            current_type = mesh->type();
            current_mat = mat;
            m_drawlist.emplace_back(GL_TRIANGLES, mesh->type(),
                    reinterpret_cast<const GLvoid*>(indirect), 0, 0);
        }
        {
            // update current drawcall (textures & bounding box)
            auto& cmd = m_drawlist.back();
            cmd.drawcount++;
            cmd.aabb.expandBy(g->getBoundingBox());
            if (mat->hasDiffuseTexture()) {
                const auto unit = core::bindings::DIFFUSE_TEX_UNIT;
                const GLuint tex = *mat->getDiffuseTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasSpecularTexture()) {
                const auto unit = core::bindings::SPECULAR_TEX_UNIT;
                const GLuint tex = *mat->getSpecularTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasEmissiveTexture()) {
                const auto unit = core::bindings::EMISSIVE_TEX_UNIT;
                const GLuint tex = *mat->getEmissiveTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasGlossyTexture()) {
                const auto unit = core::bindings::GLOSSY_TEX_UNIT;
                const GLuint tex = *mat->getGlossyTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasAlphaTexture()) {
                const auto unit = core::bindings::ALPHA_TEX_UNIT;
                const GLuint tex = *mat->getAlphaTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasNormalTexture()) {
                const auto unit = core::bindings::NORMAL_TEX_UNIT;
                const GLuint tex = *mat->getNormalTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
            if (mat->hasAmbientTexture()) {
                const auto unit = core::bindings::AMBIENT_TEX_UNIT;
                const GLuint tex = *mat->getAmbientTexture();
                assert(cmd.textures[unit] == 0 || cmd.textures[unit] == tex);
                cmd.textures[unit] = tex;
            }
        }
        indirect += sizeof(DrawElementsIndirectCommand);
    }

    // upload draw calls
    m_indirect_buffer = gl::Buffer();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER,
            static_cast<GLsizeiptr>(m_geometry.size() * sizeof(DrawElementsIndirectCommand)),
            indirectCmds.data(), 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    // make every side of the bounding box equally long
    const auto maxExtend = m_scene_bbox.maxExtend();
    const auto dist = (m_scene_bbox.pmax[maxExtend] - m_scene_bbox.pmin[maxExtend]) * .5f;
    const auto center = m_scene_bbox.center();
    for (int i = 0; i < 3; ++i) {
        m_scene_bbox.pmin[i] = center[i] - dist;
        m_scene_bbox.pmax[i] = center[i] + dist;
    }
    // make bounding box 5% biffer
    m_scene_bbox.pmin -= .05f * dist;
    m_scene_bbox.pmax += .05f * dist;

    LOG_INFO("Scene bounding box: [",
            m_scene_bbox.pmin.x, ", ",
            m_scene_bbox.pmin.y, ", ",
            m_scene_bbox.pmin.z, "] -> [",
            m_scene_bbox.pmax.x, ", ",
            m_scene_bbox.pmax.y, ", ",
            m_scene_bbox.pmax.z, "]");

    m_voxelize_cam->setLeft(m_scene_bbox.pmin.x);
    m_voxelize_cam->setRight(m_scene_bbox.pmax.x);
    m_voxelize_cam->setBottom(m_scene_bbox.pmin.y);
    m_voxelize_cam->setTop(m_scene_bbox.pmax.y);
    m_voxelize_cam->setZNear(-m_scene_bbox.pmin.z);
    m_voxelize_cam->setZFar(-m_scene_bbox.pmax.z);
    // set view matrix to identity
    m_voxelize_cam->setPosition(glm::dvec3(0.));
    m_voxelize_cam->setOrientation(glm::dquat());
    assert(m_voxelize_cam->getViewMatrix() == glm::dmat4(1.0));

}

/****************************************************************************/

void Renderer::createVoxelList()
{

    LOG_INFO("\n###########################\n"
               "# Voxel Fragment Creation #\n"
               "###########################");

    m_voxelize_timer->start();

    auto* old_cam = core::res::cameras->getDefaultCam();
    core::res::cameras->makeDefault(m_voxelize_cam);

    int num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));

    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, num_voxels);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, num_voxels);
    glViewport(0, 0, num_voxels, num_voxels);


    GLuint voxel_prog = m_voxel_prog;

    GLint loc;
    loc = glGetUniformLocation(voxel_prog, "uNumVoxels");
    glProgramUniform1i(voxel_prog, loc, num_voxels);

    // reset counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    const GLuint zero = 0;
    glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint),
            GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // setup
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, 0, sizeof(GLuint));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL_FRAGMENTS, m_voxelBuffer);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);

    renderGeometry(voxel_prog, false, nullptr);
    //glMemoryBarrier(GL_ALL_BARRIER_BITS);
    gl::printInfo();

    // TODO move to shader
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &m_numVoxelFrag);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    m_voxelize_timer->stop();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
    core::res::cameras->makeDefault(old_cam);


    LOG_INFO("Number of Entries in Voxel Fragment List: ", m_numVoxelFrag, "/", vars.max_voxel_fragments);

    if (m_numVoxelFrag > vars.max_voxel_fragments) {
        LOG_ERROR("TOO MANY VOXEL FRAGMENTS!");
    }
}

/****************************************************************************/

void Renderer::buildVoxelTree()
{

    LOG_INFO("\n###########################\n"
               "#     Octree Creation     #\n"
               "###########################");

    m_tree_timer->start();

    // calculate max invocations for compute shader to get all the voxel fragments
    const unsigned int flag_num_workgroups = (m_numVoxelFrag + FLAG_PROG_LOCAL_SIZE - 1) / FLAG_PROG_LOCAL_SIZE;

    const int num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));

    const GLuint flag_prog = m_octreeNodeFlag_prog;
    const GLuint alloc_prog = m_octreeNodeAlloc_prog;
    const GLuint init_prog = m_octreeNodeInit_prog;

    // uniforms
    glProgramUniform1ui(flag_prog, 0, m_numVoxelFrag);
    glProgramUniform1ui(flag_prog, 1, vars.voxel_octree_levels);
    glProgramUniform1ui(flag_prog, 3, static_cast<GLuint>(num_voxels));

    float scene_bbox[6] = {m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z,
                     m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z};
    glProgramUniform3fv(flag_prog, 4, 2, scene_bbox);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL_FRAGMENTS, m_voxelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL_NODE_INFO, m_octreeInfoBuffer);
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer, sizeof(GLuint), sizeof(GLuint));

    unsigned int previously_allocated = 8; // only root node was allocated
    unsigned int numAllocated = 8; // we're only allocating one block of 8 nodes, so yeah, 8;
    unsigned int start_node = 0;

    m_tree_levels.clear();

    float quarterVoxelSize = (m_scene_bbox.pmax.x - m_scene_bbox.pmin.x) / 2.f;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        quarterVoxelSize *= .5f;

        // save index of first node and number of nodes in current level
        m_tree_levels.emplace_back(start_node, previously_allocated);

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

            // set position in node info buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeInfoBuffer);
            const glm::vec3 midpoint = .5f * (m_scene_bbox.pmax + m_scene_bbox.pmin);
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_RGB32F, 0, 3 * sizeof(GLfloat),
                    GL_RGB, GL_FLOAT, glm::value_ptr(midpoint));
            // clear rest of info
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 3 * sizeof(GLfloat),
                    8 * sizeof(VoxelNodeInfo) - 3 * sizeof(GLfloat), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
            glFlush();
            glFinish();

        } else {
            glUseProgram(flag_prog);

            glProgramUniform1ui(flag_prog, 2, i);

            glDispatchCompute(flag_num_workgroups, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFlush();
            glFinish();
        }
        if (i + 1 == vars.voxel_octree_levels) {
            // no need to allocate more nodes
            break;
        }

        /*
         *  allocate child nodes
         */
        glUseProgram(alloc_prog);

        glProgramUniform1ui(alloc_prog, 0, previously_allocated);
        glProgramUniform1ui(alloc_prog, 1, start_node);

        GLuint alloc_workgroups = (previously_allocated + ALLOC_PROG_LOCAL_SIZE - 1) / ALLOC_PROG_LOCAL_SIZE;
        glDispatchCompute(alloc_workgroups, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        /*
         * Init child nodes
         */
        glUseProgram(init_prog);

        glProgramUniform1ui(init_prog, 0, previously_allocated);
        glProgramUniform1ui(init_prog, 1, start_node);
        glProgramUniform1f(init_prog, 2, quarterVoxelSize);

        GLuint init_workgroups = (previously_allocated + INIT_PROG_LOCAL_SIZE - 1) / INIT_PROG_LOCAL_SIZE;
        glDispatchCompute(init_workgroups, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

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

    LOG_INFO(":: Total Nodes created: ", numAllocated, "/", vars.max_voxel_nodes);
    if (numAllocated > vars.max_voxel_nodes) {
        LOG_ERROR("More nodes allocated than allowed!!! (", numAllocated,
                "/", vars.max_voxel_nodes, ")");
    }

    glMemoryBarrier(GL_ALL_BARRIER_BITS);


    // inject direct lighting
    {
        const unsigned int start = static_cast<unsigned int>(m_tree_levels.back().first);
        const unsigned int count = static_cast<unsigned int>(m_tree_levels.back().second);
        const GLuint inject_prog = m_inject_lighting_prog;
        glUseProgram(inject_prog);
        glUniform1ui(0, count);
        glUniform1ui(1, start);
        if (m_shadowsEnabled) {
            glUniform1ui(2, 1);
        } else {
            glUniform1ui(2, 0);
        }
        glBindImageTexture(0, m_brick_texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        const GLuint inject_workgroups = (count + INJECT_PROG_LOCAL_SIZE - 1) / INJECT_PROG_LOCAL_SIZE;
        glDispatchCompute(inject_workgroups, 1, 1);
        //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    // mipmap
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glBindImageTexture(0, m_brick_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    distributeToNeighbors(m_tree_levels.back(), true);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    for (int i = static_cast<int>(vars.voxel_octree_levels) - 2; i >= 0; i--) {
        const unsigned int start = static_cast<unsigned int>(m_tree_levels[static_cast<size_t>(i)].first);
        const unsigned int count = static_cast<unsigned int>(m_tree_levels[static_cast<size_t>(i)].second);
        const GLuint workgroups = (count + MIPMAP_PROG_LOCAL_SIZE - 1) / MIPMAP_PROG_LOCAL_SIZE;

        glUseProgram(m_mipmap_prog);
        glUniform1ui(0, count);
        glUniform1ui(1, start);
        glDispatchCompute(workgroups, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        distributeToNeighbors(m_tree_levels[static_cast<size_t>(i)], true);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

}

/****************************************************************************/

void Renderer::render(const Options & options)
{
    if (m_geometry.empty())
        return;
    assert(options.treeLevels > 0);

    m_options = options;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();
    core::res::lights->bind();

    //populateGBuffer();

    if (m_rebuildTree) {
        // only render shadowmaps once
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        renderShadowmaps();

        createVoxelList();
        buildVoxelTree();
        m_rebuildTree = false;
    }


    if (options.renderVoxelColors) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        debugRenderTree(true, options.debugLevel, true);
        glDisable(GL_BLEND);
    } else if (m_options.renderVoxelBoxes) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        debugRenderTree(false, options.debugLevel, m_options.renderVoxelBoxesColored);
        renderGeometry(m_diffuse_prog, false, core::res::cameras->getDefaultCam());
    } else {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        m_gbuffer.bindFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderGeometry(m_vertexpulling_prog, false, core::res::cameras->getDefaultCam());

        m_gbuffer.bindForShading();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // direct lighting
        if (options.renderDirectLighting) {
            glUseProgram(m_direct_lighting_prog);
            glUniform1i(0, m_shadowsEnabled);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }


        // indirect lighting
        if (options.renderConeTracing) {

            glActiveTexture(GL_TEXTURE0 + core::bindings::BRICK_TEX);
            glBindTexture(GL_TEXTURE_3D, m_brick_texture);
            glUseProgram(m_conetracing_prog);

            //const auto voxelDim = static_cast<unsigned int>(std::pow(2, options.treeLevels - 1));
            const auto voxelDim = static_cast<unsigned int>(std::pow(2, options.treeLevels - 1));
            //const auto voxelDim = static_cast<unsigned int>(std::pow(2, options.debugLevel));
            glUniform1ui(1, voxelDim);
            glUniform3f(2, m_scene_bbox.pmin.x, m_scene_bbox.pmin.y, m_scene_bbox.pmin.z);
            glUniform3f(3, m_scene_bbox.pmax.x, m_scene_bbox.pmax.y, m_scene_bbox.pmax.z);
            glUniform1ui(4, static_cast<unsigned int>(vars.screen_width));
            glUniform1ui(5, static_cast<unsigned int>(vars.screen_height));
            glUniform1ui(6, static_cast<unsigned int>(options.treeLevels));
            //glUniform1ui(6, static_cast<unsigned int>(options.debugLevel + 1));
            glUniform1ui(7, static_cast<unsigned int>(m_options.diffuseConeGridSize));
            glUniform1ui(8, static_cast<unsigned int>(m_options.specularConeSteps));
            glUniform1ui(9, static_cast<unsigned int>(m_options.diffuseConeSteps));
            glUniform1f(10, m_options.angleModifier);
            glUniform1f(11, m_options.diffuseModifier);
            glUniform1f(12, m_options.specularModifier);
            if (m_options.normalizeOutput) {
                glUniform1ui(13, 1);
            } else {
                glUniform1ui(13, 0);
            }

            glDrawArrays(GL_TRIANGLES, 0, 3);

        }

        m_gbuffer.unbindFramebuffer();
        m_gbuffer.blit();
        glDisable(GL_BLEND);

    }

    if (options.renderBBoxes)
        renderBoundingBoxes();
}

/****************************************************************************/

void Renderer::renderGeometry(const GLuint prog, const bool depthOnly,
        const core::Camera* cam)
{

    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    glUseProgram(prog);
    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (cam != nullptr && !cam->inFrustum(cmd.aabb))
            continue;

        // bind textures
        if (cmd.textures[core::bindings::ALPHA_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::ALPHA_TEX_UNIT;
            GLuint tex = cmd.textures[unit];
            if (textures[unit] != tex) {
                textures[unit] = tex;
                glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
            }
        }
        if (!depthOnly) {
            if (cmd.textures[core::bindings::DIFFUSE_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::DIFFUSE_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
            if (cmd.textures[core::bindings::SPECULAR_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::SPECULAR_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
            if (cmd.textures[core::bindings::GLOSSY_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::GLOSSY_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
            if (cmd.textures[core::bindings::NORMAL_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::NORMAL_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
            if (cmd.textures[core::bindings::EMISSIVE_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::EMISSIVE_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
            if (cmd.textures[core::bindings::AMBIENT_TEX_UNIT] != 0) {
                unsigned int unit = core::bindings::AMBIENT_TEX_UNIT;
                GLuint tex = cmd.textures[unit];
                if (textures[unit] != tex) {
                    textures[unit] = tex;
                    glBindMultiTextureEXT(GL_TEXTURE0 + unit, GL_TEXTURE_2D, tex);
                }
            }
        }

        // enable/disable shadow
        auto loc = glGetUniformLocation(prog, "u_shadowsEnabled");
        if(loc >= 0)
            glUniform1i(loc, m_shadowsEnabled);

        glMultiDrawElementsIndirect(cmd.mode, cmd.type,
                cmd.indirect, cmd.drawcount, cmd.stride);
    }
}

/****************************************************************************/

void Renderer::renderBoundingBoxes()
{
    if (m_geometry.empty())
        return;

    GLuint prog = m_bbox_prog;
    glUseProgram(prog);
    glBindVertexArray(m_bbox_vao);

    glDisable(GL_DEPTH_TEST);

    for (const auto* g : m_geometry) {
        glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE,
                nullptr, 1, 0, g->getIndex());
    }
}

/****************************************************************************/

void Renderer::initBBoxStuff()
{
    // indices
    GLubyte indices[] = {
                        // wireframe bounding box
                         0, 1,
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
                         6, 7, // 24 indices

                        // solid bounding box
                         3, 1, 0,
                         2, 3, 0,
                         6, 4, 5,
                         7, 6, 5,
                         2, 0, 4,
                         6, 2, 4,
                         7, 5, 1,
                         3, 7, 1,
                         3, 2, 6,
                         7, 3, 6,
                         5, 4, 0,
                         1, 5, 0 // 36 indices
    };

    glBindVertexArray(m_bbox_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bbox_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    core::res::shaders->registerShader("bbox_vert", "basic/bbox.vert", GL_VERTEX_SHADER);
    core::res::shaders->registerShader("bbox_frag", "basic/bbox.frag", GL_FRAGMENT_SHADER);
    m_bbox_prog = core::res::shaders->registerProgram("bbox_prog", {"bbox_vert", "bbox_frag"});
}

/****************************************************************************/

void Renderer::debugRenderTree(const bool solid, const int level, const bool colored)
{
    GLuint prog = m_voxel_bbox_prog;
    glUseProgram(prog);
    glBindVertexArray(m_bbox_vao);

    glEnable(GL_DEPTH_TEST);

    const int num_voxels = static_cast<int>(std::pow(2.0, level));
    const float halfSize = (m_scene_bbox.pmax.x - m_scene_bbox.pmin.x) /
        static_cast<float>(2 * num_voxels);
    glProgramUniform1f(prog, 0, halfSize);
    glProgramUniform1ui(prog, 1, colored);
    glBindImageTexture(0, m_brick_texture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);

    const auto& level_info = m_tree_levels[static_cast<std::size_t>(level)];

    if (!solid) {
        glDrawElementsInstancedBaseInstance(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr,
                level_info.second,
                static_cast<GLuint>(level_info.first));
    } else {
        glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE,
                reinterpret_cast<void*>(24),
                level_info.second,
                static_cast<GLuint>(level_info.first));
    }

}

/****************************************************************************/

void Renderer::markTreeInvalid()
{
    m_rebuildTree = true;
}

/****************************************************************************/

void Renderer::renderShadowmaps()
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    GLuint prog;
    if (core::res::lights->getNumShadowMapsUsed() > 0) {
        prog = m_2d_shadow_prog;
        core::res::lights->setupForShadowMapRendering();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderGeometry(prog, true, nullptr);
    }

    if (core::res::lights->getNumShadowCubeMapsUsed() > 0) {
        prog = m_cube_shadow_prog;
        core::res::lights->setupForShadowCubeMapRendering();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderGeometry(prog, true, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vars.screen_width, vars.screen_height);
}

/****************************************************************************/

const core::AABB& Renderer::getSceneBBox() const
{
    return m_scene_bbox;
}

/****************************************************************************/

void Renderer::distributeToNeighbors(const std::pair<int, int>& level, const bool average)
{
    // m_brick_texture needs to be bound to image unit 0 GL_READ_WRITE
    const unsigned int start = static_cast<unsigned int>(level.first);
    const unsigned int count = static_cast<unsigned int>(level.second);

    const GLuint dist_prog = m_dist_to_neighbors_prog;
    const GLuint workgroups = (count + TO_NEIGHBORS_PROG_LOCAL_SIZE - 1) / TO_NEIGHBORS_PROG_LOCAL_SIZE;
    glUseProgram(dist_prog);

    glUniform1ui(0, count);
    glUniform1ui(1, start);
    glUniform1i(5, static_cast<int>(average));

    // x+
    glUniform1i(2, 1);
    glUniform1i(3, 1);
    glUniform3i(4, 1, 2, 0);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // x-
    glUniform1i(2, 0);
    glUniform1i(3, -1);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // y+
    glUniform1i(2, 3);
    glUniform1i(3, 1);
    glUniform3i(4, 0, 2, 1);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // y-
    glUniform1i(2, 2);
    glUniform1i(3, -1);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // z+
    glUniform1i(2, 5);
    glUniform1i(3, 1);
    glUniform3i(4, 0, 1, 2);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // z-
    glUniform1i(2, 4);
    glUniform1i(3, -1);
    glDispatchCompute(workgroups, 1, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

/****************************************************************************/
