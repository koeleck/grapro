#include "camera.h"

namespace import
{

/****************************************************************************/

void Camera::makePointersRelative()
{
    const char* offset = reinterpret_cast<const char*>(this);
    const char* pos = reinterpret_cast<const char*>(name);

    if (pos < offset)
        return;

    name = reinterpret_cast<const char*>(pos - offset);
}

/****************************************************************************/

void Camera::makePointersAbsolute()
{
    char* offset = reinterpret_cast<char*>(this);
    std::size_t pos = reinterpret_cast<std::size_t>(name);

    if (pos > reinterpret_cast<std::size_t>(offset))
        return;

    name = reinterpret_cast<const char*>(offset + pos);
}

/****************************************************************************/

} // namespace import
