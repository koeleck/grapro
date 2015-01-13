#ifndef RENDERER_H
#define RENDERER_H

#include "rendererinterface.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

class Renderer : public RendererInterface
{
public:
    Renderer(core::TimerArray& timer_array);
    ~Renderer();

    virtual void render(unsigned int, bool = false, bool = false, bool = false);

private:

    void genAtomicBuffer();
    void genVoxelBuffer();
    void genOctreeNodeBuffer();

    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeLeafStore_prog;
    gl::Buffer                          m_atomicCounterBuffer;
    gl::Buffer                          m_voxelBuffer;
    gl::Buffer                          m_octreeNodeBuffer;

};

#endif // RENDERER_H

