#ifndef OFFSCREEN_BUFFER
#define OFFSCREEN_BUFFER

#include "gl/gl_objects.h"

struct OffscreenBuffer
{
    OffscreenBuffer(int width, int height);

    gl::Framebuffer     m_framebuffer;
    gl::Texture         m_colorbuffer;
    gl::Texture         m_depthbuffer;
    int                 m_width;
    int                 m_height;
    int                 m_levels;
};

#endif // OFFSCREEN_BUFFER

