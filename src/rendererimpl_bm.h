#ifndef RENDERERIMPLBM_H
#define RENDERERIMPLBM_H

#include "rendererinterface.h"

class RendererImplBM
  : public RendererInterface
{
public:

    explicit RendererImplBM(core::TimerArray&, unsigned int);
    ~RendererImplBM();

    virtual void render(unsigned int, unsigned int, bool = false,
                        bool = false, bool = false, bool = false);

private:

    virtual void initShaders();
    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

    void initAmbientOcclusion();
    void renderAmbientOcclusion() const;

    void resetAtomicBuffer() const;

	// ambient occlusion
    core::Program                       m_ssq_ao_prog;

};

#endif // RENDERER_H

