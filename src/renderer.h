#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <unordered_map>
#include "core/instance.h"
#include "core/program.h"
#include "gl/opengl.h"
#include "offscreen_buffer.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void setGeometry(std::vector<const core::Instance*> geometry);
    void render(bool renderBBoxes = false, bool hiz = false, int level = 0);
    void renderBoundingBoxes();

private:
    struct DrawCmd;

    void initBBoxStuff();

    void generateDrawcalls(const OffscreenBuffer& buffer);
    void downsampleDepthBuffer(const OffscreenBuffer& buffer);

    std::vector<const core::Instance*>  m_geometry;
    std::vector<DrawCmd>                m_drawlist;

    OffscreenBuffer                     m_offscreen_buffer;

    core::Program                       m_earlyz_prog;
    core::Program                       m_debugtex_prog;

    gl::VertexArray                     m_bbox_vao;
    gl::Buffer                          m_bbox_buffer;
    core::Program                       m_bbox_prog;

    core::Program                       m_vertexpulling_prog;
    gl::VertexArray                     m_vertexpulling_vao;
    gl::Buffer                          m_instanceIDs;
    gl::Buffer                          m_indirect_buffer;
    gl::Buffer                          m_bbox_indirect_buffer;

    // occlusion culling
    core::Program                       m_oc_downsample_prog;
    gl::Texture                         m_hiz_tex;
    core::Program                       m_oc_culling_prog;
};

#endif // RENDERER_H

