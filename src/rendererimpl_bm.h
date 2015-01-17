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

    void resetBuffer(const gl::Buffer &, int) const;
    void resetAtomicBuffer() const;

	// cone tracing
    core::Program                       m_ssq_ao_prog;

    core::Program                       m_gbuffer_prog;
    gl::Framebuffer                     m_gbuffer_FBO;

    gl::Texture                         m_tex_position; // 0
    gl::Texture                         m_tex_normal;   // 1
    gl::Texture                         m_tex_color;    // 2
    gl::Texture                         m_tex_depth;    // 3

    GLuint                              m_v_ssq = 0;    // 4

    gl::VertexArray                     m_vao_ssq;
    gl::Buffer                          m_vbo_ssq;

};

#endif // RENDERER_H

