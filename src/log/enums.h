#ifndef LOG_ENUMS_H
#define LOG_ENUMS_H


namespace logging
{

enum class Severity : int {
    Info,
    Warning,
    Error,
    Critical,
    Ignore
};

// Create Tags from tags.inl
#ifdef LOG_TAG
#error LOG_TAG already defined
#else
#define LOG_TAG(T) T ,
enum class Tag : int {
#include "tags.def"
};
#undef LOG_TAG
#endif // LOG_TAG

/*!
 * \brief Get string representations of tag.
 * \param t Tag enum.
 * \return String.
 */
const char* tag2str(Tag t) noexcept;

/*!
 * \brief Get string representations of severity.
 * \param s Severity enum.
 * \return String.
 */
const char* severity2str(Severity s) noexcept;

} // namespace logging

// TODO avoid 'using' in header
using logtag = logging::Tag;

#endif // LOG_ENUMS_H
