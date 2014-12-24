#include "node.h"

namespace import
{

/****************************************************************************/

void Node::makePointersRelative()
{
    const char* offset = reinterpret_cast<const char*>(this);
    const char* pos = reinterpret_cast<const char*>(name);

    if (pos < offset)
        return;

    name = reinterpret_cast<const char*>(pos - offset);
}

/****************************************************************************/

void Node::makePointersAbsolute()
{
    const char* offset = reinterpret_cast<const char*>(this);
    const std::size_t pos = reinterpret_cast<std::size_t>(name);

    if (reinterpret_cast<const char*>(pos) > offset)
        return;

    name = reinterpret_cast<const char*>(offset + pos);
}

/****************************************************************************/

} // namespace import

