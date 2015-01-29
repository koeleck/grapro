#ifndef RENDERER_H
#define RENDERER_H

/****************************************************************************/

struct Options
{
    int treeLevels;
    int debugLevel;
    bool renderBBoxes;
    bool renderVoxelBoxes;
    bool renderVoxelBoxesColored;
    bool renderVoxelColors;
    bool renderSmoothColors;
    bool renderAO;
    bool renderIndirectDiffuse;
    bool renderIndirectSpecular;
    bool renderConeTracing;
    bool debugGBuffer;
    bool renderDirectLighting;
    float aoWeight;
    int diffuseConeGridSize;
    int diffuseConeSteps;
    int specularConeSteps;
    bool debugOutput;
    float angleModifier;
    float diffuseModifier;
    float specularModifier;
    bool normalizeOutput;
};

/****************************************************************************/

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "core/instance.h"
#include "core/program.h"
#include "gl/opengl.h"
#include "core/aabb.h"
#include "core/camera.h"
#include "core/timer_array.h"
#include "gbuffer.h"

class Renderer
{
public:
    Renderer(int width, int height, core::TimerArray& timer_array);
    ~Renderer();

    void setGeometry(std::vector<const core::Instance*> geometry);
    void render(const Options &);

    void resize(int width, int height);

    void markTreeInvalid();
    void toggleShadows() { m_shadowsEnabled = !m_shadowsEnabled; }

    const core::AABB& getSceneBBox() const;

private:
    struct DrawCmd
    {
        DrawCmd();
        DrawCmd(GLenum, GLenum, const GLvoid*, GLsizei, GLsizei);

        GLuint                  textures[core::bindings::NUM_TEXT_UNITS];
        GLenum                  mode;
        GLenum                  type;
        const GLvoid*           indirect;
        GLsizei                 drawcount;
        GLsizei                 stride;
        core::AABB              aabb;
    };

    void initBBoxStuff();

    void renderGeometry(GLuint prog, bool depthOnly,
            const core::Camera* cam);
    void performOcclusionCulling();
    void debugRenderTree(bool solid, int level, bool colored, bool smooth);
    void renderBoundingBoxes();
    void renderShadowmaps();
    void distributeToNeighbors(const std::pair<int, int>& level, bool average);


    void createVoxelList();
    void buildVoxelTree();

    GBuffer                             m_gbuffer;
    core::AABB                          m_scene_bbox;
    std::vector<const core::Instance*>  m_geometry;
    std::vector<DrawCmd>                m_drawlist;
    DrawCmd                             m_bbox_draw_cmd;
    gl::Buffer                          m_indirect_buffer;

    // lights
    core::Program                       m_2d_shadow_prog;
    core::Program                       m_cube_shadow_prog;
    bool                                m_shadowsEnabled;


    gl::VertexArray                     m_bbox_vao;
    gl::Buffer                          m_bbox_buffer;
    core::Program                       m_bbox_prog;

    core::Program                       m_vertexpulling_prog;
    core::Program                       m_occlusionculling_prog;
    gl::VertexArray                     m_vertexpulling_vao;

    core::OrthogonalCamera*             m_voxelize_cam;
    core::Program                       m_voxel_prog;
    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeNodeInit_prog;
    core::Program                       m_inject_lighting_prog;
    core::Program                       m_dist_to_neighbors_prog;
    core::Program                       m_mipmap_prog;
    gl::Buffer                          m_atomicCounterBuffer;
    gl::Buffer                          m_voxelBuffer;
    gl::Buffer                          m_octreeNodeBuffer;
    gl::Buffer                          m_octreeInfoBuffer;
    gl::Texture                         m_brick_texture;
    glm::ivec3                          m_brick_texture_size;
    gl::Framebuffer                     m_voxelizationFBO;
    unsigned int                        m_numVoxelFrag;
    std::vector<std::pair<int, int>>    m_tree_levels;
    bool                                m_rebuildTree;
    core::TimerArray&                   m_timers;
    core::GPUTimer*                     m_tree_timer;
    core::GPUTimer*                     m_voxelize_timer;

    // options
    Options                             m_options;

    // deferred
    core::Program                       m_direct_lighting_prog;

    // cone tracing
    core::Program                       m_conetracing_prog;

    // debug voxeltree
    core::Program                       m_voxel_bbox_prog;
    core::Program                       m_diffuse_prog;

    // debug
    core::Program                       m_debug_tex_prog;
};

#endif // RENDERER_H

/****************************************************************************/
