#ifndef IMPORT_IMAGE_H
#define IMPORT_IMAGE_H

#include <string>
#include <FreeImage.h>
#include "gl/gl_sys.h"

namespace import
{

class Image
{
public:
    Image() noexcept;

    explicit Image(const std::string& filename);

    Image(FREE_IMAGE_TYPE type, int width, int height,
            int bpp);

    Image(const void* bits, int width, int height, int pitch,
            unsigned int bpp, bool topdown = false);

    Image(const Image& other);

    Image(Image&& other) noexcept;

    ~Image();

    Image& operator=(const Image& other) &;

    Image& operator=(Image&& other) & noexcept;

    void swap(Image& other) noexcept;

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;

    bool save(const std::string& filename,
            FREE_IMAGE_FORMAT fif,
            int flags = 0) const;

    FREE_IMAGE_TYPE type() const;

    unsigned int bpp() const;

    unsigned int width() const;

    unsigned int height() const;

    unsigned int line_width() const;

    unsigned int pitch() const;

    FREE_IMAGE_COLOR_TYPE color_type() const;

    unsigned char* bits();

    const unsigned char* bits() const;

    unsigned char* scanline(int y);

    const unsigned char* scanline(int y) const;

    Image convert_to_type(FREE_IMAGE_TYPE type,
            bool scale_linear = true) const;

    Image convert_to_greyscale() const;

    Image convert_to_24bits() const;

    Image convert_to_32bits() const;

    void convert_to_raw_bits(unsigned char* bits, int pitch,
            unsigned int bpp, bool topdown = false) const;

    GLenum gl_type() const;

    GLenum gl_format(bool normalized = true) const;

    int maxNumMipMaps() const;

    int numChannels() const;

private:
    Image(FIBITMAP* obj) noexcept;

    static void check_obj(const FIBITMAP*, const char*);

    FIBITMAP*       m_obj;
};

} // namespace import

#endif // IMPORT_IMAGE_H

