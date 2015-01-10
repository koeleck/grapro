#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "core/instance.h"
#include "core/program.h"
#include "gl/opengl.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void setGeometry(std::vector<const core::Instance*> geometry);
    void render(bool renderBBoxes = false);
    void renderBoundingBoxes();

private:
    struct DrawCmd;
    using ProgramMap = std::unordered_map<unsigned char, core::Program>;

    void initBBoxStuff();

    void genAtomicBuffer();
    void genVoxelBuffer();
    void createVoxelList();

    std::vector<const core::Instance*>  m_geometry;
    ProgramMap                          m_programs;
    std::vector<DrawCmd>                m_drawlist;

    core::Program                       m_earlyz_prog;

    gl::VertexArray                     m_bbox_vao;
    gl::Buffer                          m_bbox_buffer;
    core::Program                       m_bbox_prog;

    core::Program                       m_vertexpulling_prog;
    gl::VertexArray                     m_vertexpulling_vao;

    core::Program                       m_voxel_prog;
    GLuint                              m_atomicCounterBuffer;
    GLuint                              m_voxelBuffer;
};

#endif // RENDERER_H

