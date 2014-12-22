#include <cassert>
#include "enums.h"

namespace logging
{

namespace
{
// create tag-strings
#define LOG_TAG(T) #T ,
const char* tagStrings[]{
#include "tags.def"
};
#undef LOG_TAG

// severity strings
const char* severityStrings[] = {
    "Info",
    "Warning",
    "Error",
    "Critical",
    "Ignore"
};
} // anonymous namespace

const char* tag2str(const Tag t) noexcept
{
    assert(static_cast<unsigned int>(t) <
            (sizeof(tagStrings) / sizeof(tagStrings[0])));
    return tagStrings[static_cast<int>(t)];
}

const char* severity2str(const Severity s) noexcept
{
    assert(static_cast<unsigned int>(s) <
            (sizeof(severityStrings) / sizeof(severityStrings[0])));
    return severityStrings[static_cast<int>(s)];
}

} // namespace logging
