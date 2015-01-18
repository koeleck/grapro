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
#include "core/light_manager.h"
#include "core/texture.h"
#include "log/log.h"
#include "framework/vars.h"
#include "voxel.h"

/****************************************************************************/

namespace
{

constexpr int FLAG_PROG_LOCAL_SIZE = 64;
constexpr int ALLOC_PROG_LOCAL_SIZE = 256;

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

Renderer::Renderer(core::TimerArray& timer_array)
  : m_numVoxelFrag{0u},
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

    // node buffer
    unsigned int max_num_nodes = 1;
    unsigned int tmp = 1;
    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {
        tmp *= 8;
        max_num_nodes += tmp;
    }
    unsigned int mem = max_num_nodes * 4 +
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

    // FBO for voxelization
    int num_voxels = static_cast<int>(std::pow(2.0, vars.voxel_octree_levels - 1));
    glBindFramebuffer(GL_FRAMEBUFFER, m_voxelizationFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, num_voxels);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, num_voxels);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Empty framebuffer is not complete (WTF?!?)");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/****************************************************************************/

Renderer::~Renderer() = default;

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
    //std::vector<GLuint> instanceIDs;
    //instanceIDs.reserve(m_geometry.size() + 1);
    //// first entry in instanceIDs is the number of instances
    //instanceIDs.push_back(static_cast<GLuint>(m_geometry.size()));

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


        if (mesh->type() != current_type || current_mat == nullptr ||
                !current_mat->isCompatible(*mat))
        {
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::VOXEL, m_voxelBuffer);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);

    renderGeometry(voxel_prog);

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
        LOG_WARNING("TOO MANY VOXEL FRAGMENTS!");
    }

    // debug
    /*
    std::vector<VoxelStruct> voxels(m_numVoxelFrag);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_numVoxelFrag * sizeof(VoxelStruct), voxels.data());

    m_voxel_bboxes.clear();
    m_voxel_bboxes.reserve(m_numVoxelFrag);
    const float dist = (m_scene_bbox.pmax.x - m_scene_bbox.pmin.x) / static_cast<float>(num_voxels);
    for (const auto& el : voxels) {
        if (glm::any(glm::greaterThan(glm::uvec3(el.position), glm::uvec3(static_cast<unsigned int>(num_voxels - 1))))) {
            LOG_ERROR("out of range!");
        }
        core::AABB bbox;
        bbox.pmin = m_scene_bbox.pmin + glm::vec3(el.position) * dist;
        bbox.pmax = bbox.pmin + dist;
        m_voxel_bboxes.emplace_back(bbox);
    }
    */

    /* SLOW!!!
    auto order = [] (const core::AABB& b0, const core::AABB& b1) -> bool
            {
                return glm::any(glm::lessThan(b0.pmin, b1.pmin)) ||
                       glm::any(glm::lessThan(b0.pmax, b1.pmax));
            };
    auto compare = [] (const core::AABB& b0, const core::AABB& b1) -> bool
            {
                return glm::all(glm::equal(b0.pmin, b1.pmin)) &&
                       glm::all(glm::equal(b0.pmax, b1.pmax));
            };

    LOG_INFO("sorting bboxes...");
    std::sort(m_voxel_bboxes.begin(), m_voxel_bboxes.end(), order);
    LOG_INFO("removing duplicate bboxes...");
    m_voxel_bboxes.erase(std::unique(m_voxel_bboxes.begin(), m_voxel_bboxes.end(), compare),
                m_voxel_bboxes.end());
    LOG_INFO("done.");
    */
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

    LOG_INFO(":: Total Nodes created: ", numAllocated);

    // TODO
    ///*
    // *  write information to leafs
    // */


    // DEBUG --- create bounding boxes
    std::vector<GLuint> nodes(numAllocated);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numAllocated * sizeof(GLuint), nodes.data());

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
        if (childidx == 0x80000000) {
            m_voxel_bboxes.emplace_back(bbox);
        } else if ((childidx & 0x80000000) != 0) {
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
    core::res::lights->bind();

    const auto* cam = core::res::cameras->getDefaultCam();

    GLuint textures[core::bindings::NUM_TEXT_UNITS] = {0,};

    glUseProgram(prog);
    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
    for (const auto& cmd : m_drawlist) {
        // Frustum Culling
        if (!cam->inFrustum(cmd.aabb))
            continue;

        // bind textures
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
        if (cmd.textures[core::bindings::ALPHA_TEX_UNIT] != 0) {
            unsigned int unit = core::bindings::ALPHA_TEX_UNIT;
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

        glMultiDrawElementsIndirect(cmd.mode, cmd.type,
                cmd.indirect, cmd.drawcount, cmd.stride);
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
    //glDisable(GL_DEPTH_TEST);

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

void Renderer::markTreeInvalid()
{
    m_rebuildTree = true;
}

/****************************************************************************/

