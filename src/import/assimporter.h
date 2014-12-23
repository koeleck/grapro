#ifndef IMPORT_ASSIMPORTER_H
#define IMPORT_ASSIMPORTER_H

#include <memory>
#include <string>

namespace import
{

struct Scene;

std::unique_ptr<Scene> assimport(const std::string& filename);

} // namespace import

#endif // IMPORT_ASSIMPORTER_H

