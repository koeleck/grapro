#include "light.h"

namespace import
{

/****************************************************************************/

void Light::makePointersRelative()
{
    const char* offset = reinterpret_cast<const char*>(this);
    const char* pos = reinterpret_cast<const char*>(name);

    if (pos < offset)
        return;

    name = reinterpret_cast<const char*>(pos - offset);
}

/****************************************************************************/

void Light::makePointersAbsolute()
{
    const char* offset = reinterpret_cast<char*>(this);
    const std::size_t pos = reinterpret_cast<std::size_t>(name);

    if (pos > reinterpret_cast<std::size_t>(offset))
        return;

    name = reinterpret_cast<const char*>(offset + pos);
}

/****************************************************************************/

} // namespace import

