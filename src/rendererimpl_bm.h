#ifndef RENDERERIMPLBM_H
#define RENDERERIMPLBM_H

#include "rendererinterface.h"

class RendererImplBM
  : public RendererInterface
{
public:

    explicit RendererImplBM(core::TimerArray&, unsigned int);
    ~RendererImplBM();

    virtual void render(unsigned int, bool = false, bool = false, bool = false);

private:

    void initShaders();
    void recreateBuffer(gl::Buffer &, size_t);
    void resetBuffer(const gl::Buffer &, int);
    void resetAtomicBuffer();

    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeLeafStore_prog;
    core::Program                       m_octreeMipMap_prog;
    gl::Buffer                          m_atomicCounterBuffer;
    gl::Buffer                          m_octreeNodeColorBuffer;

};

#endif // RENDERER_H

