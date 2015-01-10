#include <cassert>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

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
    core::res::shaders->registerShader("octreeNodeFlagComp", "tree/flagoctree.comp", GL_COMPUTE_SHADER);
    m_octreeNodeFlag_prog = core::res::shaders->registerProgram("octreeNodeFlag_prog", {"octreeNodeFlagComp"});

    core::res::shaders->registerShader("octreeNodeAllocateComp", "tree/nodealloc.comp", GL_COMPUTE_SHADER);
    m_octreeNodeAllocate_prog = core::res::shaders->registerProgram("octreeNodeAllocate_prog", {"octreeNodeAllocateComp"});

    glBindVertexArray(m_vertexpulling_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, core::res::meshes->getElementArrayBuffer());
    glBindVertexArray(0);
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

    m_drawlist.clear();
    m_drawlist.reserve(m_geometry.size());
    for (const auto* g : m_geometry) {
        const auto* mesh = g->getMesh();
        const auto prog = m_programs[mesh->components()()];
        GLuint vao = core::res::meshes->getVAO(mesh);
        m_drawlist.emplace_back(g, prog, vao, mesh->mode(),
                mesh->count(), mesh->type(),
                mesh->indices(), mesh->basevertex());
    }
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

    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vars.max_voxel_fragments * sizeof(VoxelStruct), nullptr, GL_MAP_READ_BIT);
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

    glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_READ_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, core::bindings::OCTREE, m_octreeNodeBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeNodeBuffer);
}

/****************************************************************************/

void Renderer::createVoxelList()
{

    //Create an modelview-orthographic projection matrix see from X/Y/Z axis
    const glm::mat4 Ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 2.0f-1.0f, 3.0f);
    const glm::mat4 mvpX = Ortho * glm::lookAt(glm::vec3(2, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    const glm::mat4 mvpY = Ortho * glm::lookAt(glm::vec3(0, 2, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
    const glm::mat4 mvpZ = Ortho * glm::lookAt(glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto dim = static_cast<unsigned int>(std::pow(2, vars.voxel_octree_levels));
    glViewport(0, 0, dim, dim);

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
    loc = glGetUniformLocation(m_voxel_prog, "u_MVPx");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpX));
    loc = glGetUniformLocation(m_voxel_prog, "u_MVPy");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpY));
    loc = glGetUniformLocation(m_voxel_prog, "u_MVPz");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvpZ));

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
    //LOG_INFO("Number of Entries in Voxel Fragment List: ", numVoxelFrag);

    /*glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxelBuffer);
    auto vecPtr = static_cast<VoxelStruct *>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vars.max_voxel_fragments * sizeof(VoxelStruct), GL_MAP_READ_BIT));
    if (vecPtr == nullptr) LOG_INFO("NULL!");
    LOG_INFO("Position: ", vecPtr[0].position[0], " ", vecPtr[0].position[1], " ", vecPtr[0].position[2]);
    LOG_INFO("Color: ", vecPtr[0].color[0], " ", vecPtr[0].color[1], " ", vecPtr[0].color[2]);
    LOG_INFO("Normal: ", vecPtr[0].normal[0], " ", vecPtr[0].normal[1], " ", vecPtr[0].normal[2]);*/

    glViewport(0, 0, vars.screen_width, vars.screen_height);

}

/****************************************************************************/

void Renderer::buildVoxelTree()
{

    // calculate max invocations for compute shader to get all the voxel fragments
    const auto dataWidth = 1024u;
    const unsigned int dataHeight = (m_numVoxelFrag + 1023u) / dataWidth;

    // calculate max possible size of octree
    unsigned int totalNodes = 1;
    unsigned int tmp = 1;
    for(unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

        tmp *= 8;
        totalNodes += tmp;

    }

    // generate octree
    genOctreeNodeBuffer(totalNodes * sizeof(OctreeNodeStruct));

    // uniforms
    GLint loc;

    for (unsigned int i = 0; i < vars.voxel_octree_levels; ++i) {

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
        glDispatchCompute(dataWidth, dataHeight, 1); //  TO DO: less invocations!
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /*
         *  allocate child nodes
         */

        glUseProgram(m_octreeNodeAllocate_prog);

    }

}

/****************************************************************************/

void Renderer::render(const bool renderBBoxes)
{
    if (m_geometry.empty())
        return;

    core::res::materials->bind();
    core::res::instances->bind();
    core::res::meshes->bind();

    createVoxelList();
    buildVoxelTree();

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
