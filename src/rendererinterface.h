#ifndef RENDERERINTERFACE_H
#define RENDERERINTERFACE_H

/****************************************************************************/

struct Options
{
    unsigned int treeLevels;
    bool renderBBoxes;
    bool renderVoxelBoxes;
    bool renderVoxelColors;
    bool renderAO;
    bool renderIndirectDiffuse;
    bool renderIndirectSpecular;
    bool renderConeTracing;
    unsigned int aoConeGridSize;
    unsigned int aoConeSteps;
    unsigned int aoWeight;
    unsigned int diffuseConeGridSize;
    unsigned int diffuseConeSteps;
    unsigned int specularConeSteps;
    bool debugOutput;
};

/****************************************************************************/

#include <vector>
#include <unordered_map>

#include "gl/opengl.h"

#include "core/aabb.h"
#include "core/program.h"

#include "voxel.h"

namespace core {
    class TimerArray;
    class GPUTimer;
    class Instance;
    class OrthogonalCamera;
}

class RendererInterface
{
public:
    explicit RendererInterface(core::TimerArray& timer_array, unsigned int treeLevels);
    virtual ~RendererInterface();

    virtual void render(const Options & options) = 0;

    void setGeometry(std::vector<const core::Instance*> geometry);
    void markTreeInvalid() { m_rebuildTree = true; }

    const core::AABB& getSceneBBox() const { return this->m_scene_bbox; }
protected:
    struct DrawCmd;

    virtual void initShaders() = 0;
    virtual void createVoxelList(bool debug_output = false) = 0;
    virtual void buildVoxelTree(bool debug_output = false) = 0;

    void renderGeometry(GLuint prog) const;
    void renderBoundingBoxes() const;
    void renderVoxelBoundingBoxes() const;
    void renderVoxelColors() const;
    void renderToGBuffer() const;
    void renderIndirectDiffuseLighting() const;
    void renderIndirectSpecularLighting() const;
    void coneTracing() const;

    void createVoxelBBoxes(unsigned int num);
    void resizeFBO() const;
    unsigned int calculateMaxNodes() const;
    void recreateBuffer(gl::Buffer & buf, size_t size) const;

    using ProgramMap = std::unordered_map<unsigned char, core::Program>;
    ProgramMap                          m_programs;

    // geometry
    std::vector<const core::Instance*>  m_geometry;
    std::vector<DrawCmd>                m_drawlist;
    core::Program                       m_vertexpulling_prog;
    gl::VertexArray                     m_vertexpulling_vao;

    // bbox
    core::AABB                          m_scene_bbox;
    gl::VertexArray                     m_bbox_vao;
    gl::Buffer                          m_bbox_buffer;
    core::Program                       m_bbox_prog;

    // voxel bboxes for debugging
    std::vector<core::AABB>             m_voxel_bboxes;
    core::Program                       m_voxel_bbox_prog;

    // voxelization & octree
    core::OrthogonalCamera*             m_voxelize_cam;
    core::Program                       m_voxel_prog;
    gl::Buffer                          m_voxelBuffer;
    gl::Framebuffer                     m_voxelizationFBO;
    unsigned int                        m_numVoxelFrag;

    // octree
    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeMipMap_prog;
    gl::Buffer                          m_octreeNodeBuffer;
    gl::Buffer                          m_octreeNodeColorBuffer;
    gl::Buffer                          m_brickBuffer;
    bool                                m_rebuildTree;
    unsigned int                        m_treeLevels;

    // timing
    core::TimerArray&                   m_timers;
    core::GPUTimer*                     m_voxelize_timer;
    core::GPUTimer*                     m_tree_timer;
    core::GPUTimer*                     m_mipmap_timer;

    // gbuffer
    core::Program                       m_gbuffer_prog;
    gl::Framebuffer                     m_gbuffer_FBO;
    gl::Texture                         m_tex_position; // 0
    gl::Texture                         m_tex_normal;   // 1
    gl::Texture                         m_tex_color;    // 2
    gl::Texture                         m_tex_depth;    // 3
    gl::VertexArray                     m_vao_ssq;
    gl::Buffer                          m_vbo_ssq;

    // indirect
    core::Program                       m_indirectDiffuse_prog;
    core::Program                       m_indirectSpecular_prog;

    // color boxes
    core::Program                       m_colorboxes_prog;
    gl::VertexArray                     m_colorboxes_vao;
    gl::Buffer                          m_colorboxes_vbo;

    // brick texture
    gl::Texture                         m_brick_texture;
    glm::ivec3                          m_brick_texture_size;
    core::Program                       m_inject_lighting_prog;

    // other
    gl::Buffer                          m_atomicCounterBuffer;
    core::Program                       m_coneTracing_prog;
    Options                             m_options;

private:

    void initBBoxes();
    void initVertexPulling();
    void initVoxelization();
    void initVoxelBBoxes();
    void initVoxelColors();
    void initGBuffer();
    void initConeTracingPass();
    void initBrickTexture();

};

/****************************************************************************/

struct RendererInterface::DrawCmd
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

#endif // RENDERER_H

