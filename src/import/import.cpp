#include <fstream>
#include <cstring>

#include "framework/vars.h"
#include "import.h"
#include "assimporter.h"

#include <boost/filesystem.hpp>
using namespace boost::filesystem;

/***************************************************************************/

namespace import
{
namespace
{

/***************************************************************************/

constexpr int VERSION = Scene::VERSION + Light::VERSION + Material::VERSION +
        Mesh::VERSION + Node::VERSION + Texture::VERSION + Camera::VERSION +
        ASSIMPORTER_VERSION;

/***************************************************************************/

struct FileHeader
{
    char header[4];
    int version;
};

/***************************************************************************/

constexpr char HEADER_STRING[5] = "aibf";

/***************************************************************************/

std::unique_ptr<Scene> getCachedData(const std::string& scenefile)
{
    std::unique_ptr<Scene> result;

    path file = vars.cache_dir / path(scenefile);
    if (!exists(file) || !is_regular_file(file)) {
        return result;
    }

    std::ifstream is(file.c_str(), std::ios::in | std::ios::binary);
    if (!is) {
        return result;
    }

    is.seekg(0, std::ios::end);
    const std::size_t size = static_cast<std::size_t>(is.tellg());
    is.seekg(0, std::ios::beg);

    if (size < sizeof(FileHeader) + sizeof(Scene)) {
        return result;
    }

    FileHeader header;
    is.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

    if (std::strncmp(header.header, HEADER_STRING, sizeof(HEADER_STRING) - 1) != 0 ||
            header.version != VERSION)
    {
        is.close();
        std::remove(file.c_str());
        return result;
    }

    char* data = new char[size - sizeof(FileHeader)];
    result.reset(reinterpret_cast<Scene*>(data));
    is.read(data, static_cast<long>(size - sizeof(FileHeader)));
    is.close();

    if (result->size != size - sizeof(FileHeader)) {
        result.reset();
    }

    return result;
}

/***************************************************************************/

void writeDataToCache(const Scene* data, const std::string& scenefile)
{
    path file = vars.cache_dir / path(scenefile);

    if (exists(file) == false) {
        create_directories(file.parent_path());
    } else if (!is_regular_file(file)) {
        return;
    }

    std::ofstream os(file.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!os) {
        return;
    }

    FileHeader header;
    std::memcpy(header.header, HEADER_STRING, sizeof(HEADER_STRING) - 1);
    header.version = VERSION;

    os.write(reinterpret_cast<const char*>(&header), sizeof(header));
    os.write(reinterpret_cast<const char*>(data), data->size);

    os.close();
}

/***************************************************************************/

} // anonymous namespace

/***************************************************************************/

std::unique_ptr<Scene> importSceneFile(const std::string& filename)
{
    std::unique_ptr<Scene> result = getCachedData(filename);
    if (!result) {
        result = assimport(filename);
        if (!result)
            return result;

        result->makePointersRelative();
        writeDataToCache(result.get(), filename);
        result->makePointersAbsolute();
    } else {
        result->makePointersAbsolute();
    }

    path parent_dir = path(filename).parent_path();
    for (unsigned int i = 0; i < result->num_textures; ++i) {
        path img_path = parent_dir / result->textures[i]->name;
        Image* img = new Image(img_path.string<std::string>());
        if (!(*img)) {
            delete img;
            result.reset();
            break;
        }
        result->textures[i]->image = img;
    }

    return result;
}

/***************************************************************************/

} // namespace import
