#include <cstdlib>

#include "tex_formats.h"

namespace gl
{

//////////////////////////////////////////////////////////////////////////

int get_num_channels(const GLenum format) noexcept
{
    switch (format) {
    case GL_RED:
    case GL_RED_INTEGER:
    case GL_DEPTH_COMPONENT:
    case GL_STENCIL_INDEX:
        return 1;
    case GL_RG:
    case GL_RG_INTEGER:
    case GL_DEPTH_STENCIL:
        return 2;
    case GL_RGB:
    case GL_BGR:
    case GL_RGB_INTEGER:
    case GL_BGR_INTEGER:
        return 3;
    case GL_RGBA:
    case GL_BGRA:
    case GL_RGBA_INTEGER:
    case GL_BGRA_INTEGER:
        return 4;
    default:
        break;
    }
    // we should not reach this
    abort();
}

//////////////////////////////////////////////////////////////////////////

bool is_integer_format(const GLenum format) noexcept
{
    switch (format) {
    case GL_RED_INTEGER:
    case GL_RG_INTEGER:
    case GL_RGB_INTEGER:
    case GL_BGR_INTEGER:
    case GL_RGBA_INTEGER:
    case GL_BGRA_INTEGER:
        return true;

    default:
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

bool is_signed(const GLenum type) noexcept
{
    switch (type) {
    case GL_BYTE:
    case GL_SHORT:
    case GL_INT:
    case GL_FLOAT:
        return true;
    default:
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

GLenum get_suitable_internal_format(const GLenum format, const GLenum type,
        const bool use_compression) noexcept
{
    const int num_channels = get_num_channels(format);
    const bool int_format = is_integer_format(format);
    const bool snorm = is_signed(type);
    const bool has_s3tc = glewIsSupported("EXT_texture_compression_s3tc");
    const bool has_rgtc = glewIsSupported("ARB_texture_compression_rgtc");
    const bool has_bptc = glewIsSupported("ARB_texture_compression_bptc");

    GLenum internal_format;

    if (num_channels == 1) {
        if (use_compression && !int_format && has_rgtc) {
            internal_format = snorm ? GL_COMPRESSED_SIGNED_RED_RGTC1 :
                GL_COMPRESSED_RED_RGTC1;
        } else if (type == GL_FLOAT) {
            internal_format = GL_R32F;
        } else if (type == GL_INT) {
            // GL_R32_SNORM not available! Choose closest.
            internal_format = int_format ? GL_R32I : GL_R16_SNORM;
        } else if (type == GL_UNSIGNED_INT) {
            // GL_R32 not available! Choose closest.
            internal_format = int_format ? GL_R32UI : GL_R16;
        } else if (type == GL_SHORT) {
            internal_format = int_format ? GL_R16I : GL_R16_SNORM;
        } else if (type == GL_UNSIGNED_SHORT) {
            internal_format = int_format ? GL_R16UI : GL_R16;
        } else if (type == GL_BYTE) {
            internal_format = int_format ? GL_R8I : GL_R8_SNORM;
        } else {
            // just use GL_R8 for everything else
            internal_format = int_format ? GL_R8UI : GL_R8;
        }
    } else if (num_channels == 2) {
        if (use_compression && !int_format && has_rgtc) {
            internal_format = snorm ? GL_COMPRESSED_SIGNED_RG_RGTC2 :
                GL_COMPRESSED_RG_RGTC2;
        } else if (type == GL_FLOAT) {
            internal_format = GL_RG32F;
        } else if (type == GL_INT) {
            // GL_RG32_SNORM not available! Choose closest.
            internal_format = int_format ? GL_RG32I : GL_RG16_SNORM;
        } else if (type == GL_UNSIGNED_INT) {
            // GL_RG32 not available! Choose closest.
            internal_format = int_format ? GL_RG32UI : GL_RG16;
        } else if (type == GL_SHORT) {
            internal_format = int_format ? GL_RG16I : GL_RG16_SNORM;
        } else if (type == GL_UNSIGNED_SHORT) {
            internal_format = int_format ? GL_RG16UI : GL_RG16;
        } else if (type == GL_BYTE) {
            internal_format = int_format ? GL_RG8I : GL_RG8_SNORM;
        } else {
            // just use GL_RG8 for everything else
            internal_format = int_format ? GL_RG8UI : GL_RG8;
        }
    } else if (num_channels == 3) {
        if (use_compression && !int_format && has_bptc) {
            internal_format = snorm ? GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT :
                    GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        } else if (use_compression && !int_format && !snorm && has_s3tc) {
            internal_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        } else if (type == GL_FLOAT) {
            internal_format = GL_RGB32F;
        } else if (type == GL_INT) {
            // GL_RGB32_SNORM not available! Choose closest.
            internal_format = int_format ? GL_RGB32I : GL_RGB16_SNORM;
        } else if (type == GL_UNSIGNED_INT) {
            // GL_RGB32 not available! Choose closest.
            internal_format = int_format ? GL_RGB32UI : GL_RGB16;
        } else if (type == GL_SHORT) {
            internal_format = int_format ? GL_RGB16I : GL_RGB16_SNORM;
        } else if (type == GL_UNSIGNED_SHORT) {
            internal_format = int_format ? GL_RGB16UI : GL_RGB16;
        } else if (type == GL_BYTE) {
            internal_format = int_format ? GL_RGB8I : GL_RGB8_SNORM;
        } else {
            // just use GL_RGB8 for everything else
            internal_format = int_format ? GL_RGB8UI : GL_RGB8;
        }
    } else if (num_channels == 4) {
        if (use_compression && !int_format && !snorm &&
                (has_bptc || has_s3tc))
        {
            internal_format = has_bptc ? GL_COMPRESSED_RGBA_BPTC_UNORM :
                GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        } else if (type == GL_FLOAT) {
            internal_format = GL_RGBA32F;
        } else if (type == GL_INT) {
            // GL_RGBA32_SNORM not available! Choose closest.
            internal_format = int_format ? GL_RGBA32I : GL_RGBA16_SNORM;
        } else if (type == GL_UNSIGNED_INT) {
            // GL_RGBA32 not available! Choose closest.
            internal_format = int_format ? GL_RGBA32UI : GL_RGBA16;
        } else if (type == GL_SHORT) {
            internal_format = int_format ? GL_RGBA16I : GL_RGBA16_SNORM;
        } else if (type == GL_UNSIGNED_SHORT) {
            internal_format = int_format ? GL_RGBA16UI : GL_RGBA16;
        } else if (type == GL_BYTE) {
            internal_format = int_format ? GL_RGBA8I : GL_RGBA8_SNORM;
        } else {
            // just use GL_RGBA8 for everything else
            internal_format = int_format ? GL_RGBA8UI : GL_RGBA8;
        }
    } else {
        // should not be reached
        abort();
    }

    return internal_format;
}

//////////////////////////////////////////////////////////////////////////

} // namespace gl
