#ifndef RENDERERIMPLPK_H
#define RENDERERIMPLPK_H

#include "rendererinterface.h"

class RendererImplPK : public RendererInterface
{
public:
    RendererImplPK(core::TimerArray& timer_array);
    ~RendererImplPK();

    virtual void render(unsigned int, bool = false, bool = false, bool = false);

private:

    void renderGeometry(GLuint prog);

    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);


    core::Program                       m_octreeNodeFlag_prog;
    core::Program                       m_octreeNodeAlloc_prog;
    core::Program                       m_octreeLeafStore_prog;
    gl::Buffer                          m_atomicCounterBuffer;

};

#endif // RENDERERIMPLPK_H
