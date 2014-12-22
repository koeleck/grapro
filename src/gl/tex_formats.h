#ifndef GL_TEX_FORMATS_H
#define GL_TEX_FORMATS_H

#include "gl_sys.h"

namespace gl
{

int get_num_channels(GLenum format) noexcept;

bool is_integer_format(GLenum format) noexcept;

bool is_signed(GLenum type) noexcept;

GLenum get_suitable_internal_format(GLenum format, GLenum type,
        bool use_compression) noexcept;

} // namespace gl

#endif // GL_TEX_FORMATS_H
