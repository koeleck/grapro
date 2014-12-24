#ifndef IMPORT_TEXTURE_H
#define IMPORT_TEXTURE_H

namespace import
{

class Image;

struct Texture
{
    static constexpr int VERSION = 1;

    void makePointersRelative();
    void makePointersAbsolute();

    const char* name;
    Image*      image;
};

} // namespace import

#endif // IMPORT_TEXTURE_H
