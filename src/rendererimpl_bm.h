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

    virtual void initShaders();
    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

    void resetBuffer(const gl::Buffer &, int) const;
    void resetAtomicBuffer() const;

};

#endif // RENDERER_H

