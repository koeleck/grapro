#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "core/instance.h"
#include "core/program.h"
#include "gl/opengl.h"
#include "core/aabb.h"
#include "core/camera.h"
#include "core/timer_array.h"

class Renderer
{
public:
    Renderer(core::TimerArray& timer_array);
    ~Renderer();

    void setGeometry(std::vector<const core::Instance*> geometry);
    void render(bool renderBBoxes = false, bool render_octree = false);

    void markTreeInvalid();

private:
    struct DrawCmd;
    using ProgramMap = std::unordered_map<unsigned char, core::Program>;

    void initBBoxStuff();

    void renderGeometry(GLuint prog);
    void debugRenderTree();
    void renderBoundingBoxes();

    void createVoxelList();
    void buildVoxelTree();

    core::AABB                          m_scene_bbox;
    std::vector<const core::Instance*>  m_geometry;
    ProgramMap                          m_programs;
    std::vector<DrawCmd>                m_drawlist;
    gl::Buffer                          m_indirect_buffer;

    core::Program                       m_earlyz_prog;

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
    gl::Buffer                          m_atomicCounterBuffer;
    gl::Buffer                          m_voxelBuffer;
    gl::Buffer                          m_octreeNodeBuffer;
    gl::Framebuffer                     m_voxelizationFBO;
    unsigned int                        m_numVoxelFrag;
    bool                                m_rebuildTree;
    core::TimerArray&                   m_timers;
    core::GPUTimer*                     m_tree_timer;
    core::GPUTimer*                     m_voxelize_timer;

    // debug voxeltree
    std::vector<core::AABB>             m_voxel_bboxes;
    core::Program                       m_voxel_bbox_prog;
};

#endif // RENDERER_H

