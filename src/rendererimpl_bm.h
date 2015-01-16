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

    void initAmbientOcclusion();
    void renderAmbientOcclusion() const;

    void resetBuffer(const gl::Buffer &, int) const;
    void resetAtomicBuffer() const;

	// cone tracing
    core::Program                       m_gbuffer_prog;
    gl::Framebuffer                     m_gbuffer_FBO;
    gl::Texture                         m_tex_position;

};

#endif // RENDERER_H

