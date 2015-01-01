#include <cassert>
#include <cmath>
#include <utility>
#include <stdexcept>

#include "image.h"

#include "log/log.h"

/////////////////////////////////////////////////////////////////////

namespace
{
//
// FreeImage error handling
//
class FIErrorHandler
{
public:
    FIErrorHandler() noexcept
    {
        FreeImage_SetOutputMessage(errorMsg);
    }

private:
    static void errorMsg(const FREE_IMAGE_FORMAT fif,
            const char* const message) noexcept
    {
        LOG_ERROR(logtag::FreeImage, "Error: (",
                (fif != FIF_UNKNOWN ? FreeImage_GetFormatFromFIF(fif) :
                 "unkown format"), ") ",
                (message != nullptr ? message : ""));
    }
};

// Set at start of program
FIErrorHandler fiErrorHandler;

} // unnamed namespace

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

namespace import
{

/////////////////////////////////////////////////////////////////////

Image::Image() noexcept
  : m_obj{nullptr}
{
}

/////////////////////////////////////////////////////////////////////

Image::Image(FIBITMAP* const obj) noexcept
  : m_obj{obj}
{
}

/////////////////////////////////////////////////////////////////////

Image::Image(const std::string& filename)
  : m_obj{nullptr}
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename.c_str(), 0);
    if (format != FIF_UNKNOWN) {
        m_obj = FreeImage_Load(format, filename.c_str(), 0);
    }
    if (m_obj == nullptr) {
        LOG_ERROR(logtag::FreeImage, "Failed to load image file: ", filename);
        throw std::runtime_error("");
    }
}

/////////////////////////////////////////////////////////////////////

Image::Image(const FREE_IMAGE_TYPE type_, const int width_,
        const int height_, const int bpp_)
  : m_obj{nullptr}
{
    assert(width_ > 0 && height_ > 0 && bpp_ > 0);
    m_obj = FreeImage_AllocateT(type_, width_, height_, bpp_);
    if (m_obj == nullptr) {
        LOG_ERROR(logtag::FreeImage, "Failed to create image");
        throw std::runtime_error("");
    }
}

/////////////////////////////////////////////////////////////////////

Image::Image(const void* const bits_, const int width_,
        const int height_, const int pitch_, const unsigned int bpp_,
        const bool topdown)
  : m_obj{nullptr}
{
    assert(width_ > 0 && height_ > 0 && bpp_ > 0 &&
           bits_ != nullptr &&
           pitch_ >= (width_ * static_cast<int>(bpp_) / 8));
    // stupid FreeImage isn't const correct!
    void* data = const_cast<void*>(bits_);
    m_obj = FreeImage_ConvertFromRawBits(static_cast<BYTE*>(data),
            width_, height_, pitch_, bpp_,
            FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK,
            topdown);
    if (m_obj == nullptr) {
        LOG_ERROR(logtag::FreeImage, "Failed to create image");
        throw std::runtime_error("");
    }
}

/////////////////////////////////////////////////////////////////////

Image::Image(const Image& other)
  : m_obj{nullptr}
{
    if (other.m_obj == nullptr) {
        return;
    }
    m_obj = FreeImage_Clone(other.m_obj);
    if (m_obj == nullptr) {
        LOG_ERROR(logtag::FreeImage, "Failed to create copy");
        throw std::runtime_error("");
    }
}

/////////////////////////////////////////////////////////////////////

Image::Image(Image&& other) noexcept
  : m_obj{other.m_obj}
{
    other.m_obj = nullptr;
}

/////////////////////////////////////////////////////////////////////

Image::~Image()
{
    if (m_obj != nullptr) {
        FreeImage_Unload(m_obj);
        m_obj = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////

Image& Image::operator=(const Image& other) &
{
    if (this != &other) {
        Image tmp{other};
        swap(tmp);
    }
    return *this;
}

/////////////////////////////////////////////////////////////////////

Image& Image::operator=(Image&& other) & noexcept
{
    swap(other);
    return *this;
}

/////////////////////////////////////////////////////////////////////

void Image::swap(Image& other) noexcept
{
    std::swap(m_obj, other.m_obj);
}

/////////////////////////////////////////////////////////////////////

Image::operator bool() const noexcept
{
    return m_obj != nullptr;
}

/////////////////////////////////////////////////////////////////////

bool Image::operator!() const noexcept
{
    return m_obj == nullptr;
}

/////////////////////////////////////////////////////////////////////

bool Image::save(const std::string& filename,
        const FREE_IMAGE_FORMAT fif,
        const int flags) const
{
    check_obj(m_obj, "Not a valid image object");

    const auto res = FreeImage_Save(fif, m_obj, filename.c_str(), flags);
    return res != 0;
}

/////////////////////////////////////////////////////////////////////

FREE_IMAGE_TYPE Image::type() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetImageType(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned int Image::bpp() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetBPP(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned int Image::width() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetWidth(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned int Image::height() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetHeight(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned int Image::line_width() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetLine(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned int Image::pitch() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetPitch(m_obj);
}

/////////////////////////////////////////////////////////////////////

FREE_IMAGE_COLOR_TYPE Image::color_type() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetColorType(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned char* Image::bits()
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetBits(m_obj);
}

/////////////////////////////////////////////////////////////////////

const unsigned char* Image::bits() const
{
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetBits(m_obj);
}

/////////////////////////////////////////////////////////////////////

unsigned char* Image::scanline(const int y)
{
    assert(y >= 0);
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetScanLine(m_obj, y);
}

/////////////////////////////////////////////////////////////////////

const unsigned char* Image::scanline(const int y) const
{
    assert(y >= 0);
    check_obj(m_obj, "Not a valid image object");
    return FreeImage_GetScanLine(m_obj, y);
}

/////////////////////////////////////////////////////////////////////

Image Image::convert_to_type(const FREE_IMAGE_TYPE type_,
        const bool scale_linear) const
{
    check_obj(m_obj, "Not a valid image object");
    FIBITMAP* const obj = FreeImage_ConvertToType(m_obj, type_, scale_linear);
    check_obj(obj, "Failed to convert image");
    return obj;
}

/////////////////////////////////////////////////////////////////////

Image Image::convert_to_greyscale() const
{
    check_obj(m_obj, "Not a valid image object");
    if (m_obj == nullptr)
        throw std::runtime_error("Not a valid image object");
    FIBITMAP* const obj = FreeImage_ConvertToGreyscale(m_obj);
    check_obj(obj, "Failed to convert image");
    return obj;
}

/////////////////////////////////////////////////////////////////////

Image Image::convert_to_24bits() const
{
    check_obj(m_obj, "Not a valid image object");
    FIBITMAP* const  obj = FreeImage_ConvertTo24Bits(m_obj);
    check_obj(obj, "Failed to convert image");
    return obj;
}

/////////////////////////////////////////////////////////////////////

Image Image::convert_to_32bits() const
{
    check_obj(m_obj, "Not a valid image object");
    FIBITMAP* const obj = FreeImage_ConvertTo32Bits(m_obj);
    check_obj(obj, "Failed to convert image");
    return obj;
}

/////////////////////////////////////////////////////////////////////

void Image::convert_to_raw_bits(unsigned char* const bits_,
        const int pitch_, const unsigned int bpp_,
        const bool topdown) const
{
    assert(bits_ != nullptr && pitch_ > 0 && bpp_ > 0);
    check_obj(m_obj, "Not a valid image object");
    FreeImage_ConvertToRawBits(bits_, m_obj, pitch_, bpp_,
            FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK,
            topdown);
}

/////////////////////////////////////////////////////////////////////

void Image::check_obj(const FIBITMAP* ptr, const char* const msg)
{
    if (ptr == nullptr)
        throw std::runtime_error(msg);
}

/////////////////////////////////////////////////////////////////////

GLenum Image::gl_type() const
{
    check_obj(m_obj, "Not a valid image object");

    GLenum img_type = 0;

    const auto t = type();
    switch (t) {
    case FIT_BITMAP:
    {
        const auto BPP = bpp();
        const auto ct = color_type();
        switch (ct) {
        case FIC_MINISBLACK:
        case FIC_MINISWHITE:
            if (BPP == 8) {
                img_type = GL_UNSIGNED_BYTE;
            }
            break;

        case FIC_RGB:
            if (BPP == 24) {
                img_type = GL_UNSIGNED_BYTE;
            }
            break;

        case FIC_RGBALPHA:
            if (BPP == 32) {
                img_type = GL_UNSIGNED_BYTE;
            }
            break;

        case FIC_PALETTE:
        case FIC_CMYK:
        default:
            break;
        }
        if (img_type == 0) {
            throw std::runtime_error("Unsupported/Unkown format");
        }
        break;
    }

    case FIT_UINT16:
        img_type = GL_UNSIGNED_SHORT;
        break;

    case FIT_INT16:
    case FIT_RGB16:
    case FIT_RGBA16:
        img_type = GL_SHORT;
        break;

    case FIT_UINT32:
        img_type = GL_UNSIGNED_INT;
        break;

    case FIT_INT32:
        img_type = GL_INT;
        break;

    case FIT_FLOAT:
    case FIT_RGBF:
    case FIT_RGBAF:
        img_type = GL_FLOAT;
        break;

    case FIT_COMPLEX:
    case FIT_DOUBLE:
    default:
        throw std::runtime_error("Unsupported/Unkown format");
        break;
    }

    return img_type;
}

/////////////////////////////////////////////////////////////////////

GLenum Image::gl_format(const bool normalized) const
{
    check_obj(m_obj, "Not a valid image object");

    GLenum format;

    const auto t = type();
    switch (t) {
    case FIT_BITMAP:
    {
        const auto ct = color_type();
        switch (ct) {
        case FIC_MINISBLACK:
        case FIC_MINISWHITE:
            format = normalized ? GL_RED : GL_RED_INTEGER;
            break;

        case FIC_RGB:
            format = normalized ? GL_BGR : GL_BGR_INTEGER;
            break;

        case FIC_RGBALPHA:
            format = normalized ? GL_BGRA : GL_BGRA_INTEGER;
            break;

        case FIC_PALETTE:
        case FIC_CMYK:
        default:
            throw std::runtime_error("Unsupported/Unkown format");
            break;
        }
        break;
    }

    case FIT_INT16:
    case FIT_UINT16:
    case FIT_INT32:
    case FIT_UINT32:
        format = normalized ? GL_RED : GL_RED_INTEGER;
        break;

    case FIT_FLOAT:
        format = GL_RED;
        break;

    case FIT_RGB16:
        format = normalized ? GL_BGR : GL_BGR_INTEGER;
        break;

    case FIT_RGBA16:
        format = normalized ? GL_BGRA : GL_BGRA_INTEGER;
        break;

    case FIT_RGBF:
        format = GL_BGR;
        break;

    case FIT_RGBAF:
        format = GL_BGRA;
        break;

    case FIT_COMPLEX:
    case FIT_DOUBLE:
    case FIT_UNKNOWN:
    default:
        throw std::runtime_error("Unsupported/Unkown format");
        break;
    }

    return format;
}

/////////////////////////////////////////////////////////////////////

int Image::maxNumMipMaps() const
{
    check_obj(m_obj, "Not a valid image object");
    auto sz = std::max(width(), height());
    return 1 + static_cast<int>(std::floor(std::log2(sz)));
}

/////////////////////////////////////////////////////////////////////

int Image::numChannels() const
{
    check_obj(m_obj, "Not a valid image object");

    const auto t = type();
    switch (t) {
    case FIT_BITMAP:
    {
        const auto ct = color_type();
        switch (ct) {
        case FIC_MINISBLACK:
        case FIC_MINISWHITE:
            return 1;

        case FIC_RGB:
            return 3;

        case FIC_RGBALPHA:
            return 4;

        case FIC_PALETTE:
        case FIC_CMYK:
        default:
            throw std::runtime_error("Unsupported/Unkown format");
            break;
        }
        break;
    }

    case FIT_INT16:
    case FIT_UINT16:
    case FIT_INT32:
    case FIT_UINT32:
    case FIT_FLOAT:
        return 1;

    case FIT_RGB16:
    case FIT_RGBF:
        return 3;

    case FIT_RGBA16:
    case FIT_RGBAF:
        return 4;

    case FIT_COMPLEX:
    case FIT_DOUBLE:
    case FIT_UNKNOWN:
    default:
        throw std::runtime_error("Unsupported/Unkown format");
        break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

} // namespace import
