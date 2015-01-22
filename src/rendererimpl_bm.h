#ifndef RENDERERIMPLBM_H
#define RENDERERIMPLBM_H

#include "rendererinterface.h"

class RendererImplBM
  : public RendererInterface
{
public:

    explicit RendererImplBM(core::TimerArray&, unsigned int);
    ~RendererImplBM();

    virtual void render(unsigned int, bool = false, bool = false,
                        bool = false, bool = false);

private:

    virtual void initShaders();
    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

    void initAmbientOcclusion();
    void renderAmbientOcclusion() const;
    void renderVoxels() const;
    void renderShadowmaps();

    void resetAtomicBuffer() const;

	// ambient occlusion
    core::Program                       m_ssq_ao_prog;

    // lights
    core::Program m_2d_shadow_prog;
    core::Program m_cube_shadow_prog;
};

#endif // RENDERER_H

