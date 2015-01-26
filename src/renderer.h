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
    bool renderAO;
    bool renderIndirectDiffuse;
    bool renderIndirectSpecular;
    bool renderConeTracing;
    bool debugGBuffer;
    bool renderDirectLighting;
    int aoConeGridSize;
    int aoConeSteps;
    int aoWeight;
    int diffuseConeGridSize;
    int diffuseConeSteps;
    int specularConeSteps;
    bool debugOutput;
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
    struct DrawCmd;

    void initBBoxStuff();

    void renderGeometry(GLuint prog, bool depthOnly,
            const core::Camera* cam);
    void debugRenderTree(bool solid, int level, bool colored);
    void renderBoundingBoxes();
    void renderShadowmaps();
    void populateGBuffer();
    void distributeToNeighbors(const std::pair<int, int>& level, bool average);


    void createVoxelList();
    void buildVoxelTree();

    GBuffer                             m_gbuffer;
    core::AABB                          m_scene_bbox;
    std::vector<const core::Instance*>  m_geometry;
    std::vector<DrawCmd>                m_drawlist;
    gl::Buffer                          m_indirect_buffer;

    // lights
    core::Program                       m_2d_shadow_prog;
    core::Program                       m_cube_shadow_prog;
    bool                                m_shadowsEnabled = true;


    gl::VertexArray                     m_bbox_vao;
    gl::Buffer                          m_bbox_buffer;
    core::Program                       m_bbox_prog;

    core::Program                       m_vertexpulling_prog;
    gl::VertexArray                     m_vertexpulling_vao;

    core::OrthogonalCamera*             m_voxelize_cam;
    core::Program                       m_voxel_prog;
    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeLeafStore_prog;
    core::Program                       m_inject_lighting_prog;
    core::Program                       m_dist_to_neighbors_prog;
    core::Program                       m_mipmap_prog;
    core::Program                       m_mipmap2_prog;
    core::Program                       m_mipmap3_prog;
    core::Program                       m_mipmap4_prog;
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

    // debug
    core::Program                       m_debug_tex_prog;
};

#endif // RENDERER_H

/****************************************************************************/
