#ifndef LOG_LOG_H
#define LOG_LOG_H

#include <sstream>
#include "enums.h"
#include "entry.h"
#include "relay.h"

#define LOG_INFO(...) \
    ::logging::log(::logging::Severity::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) \
    ::logging::log(::logging::Severity::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) \
    ::logging::log(::logging::Severity::Error, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) \
    ::logging::log(::logging::Severity::Critical, __FILE__, __LINE__, __VA_ARGS__)

namespace logging
{

//
// functions for actually writing something:
//
namespace detail
{

// forward declarations:
template <typename T, typename... Args>
void append(std::ostringstream& out, std::vector<Tag>& tags, const T& var, Args... args);

template <typename... Args>
void append(std::ostringstream& out, std::vector<Tag>& tags, const Tag tag, Args... args);

inline
void append(std::ostringstream&    , std::vector<Tag>&);



template <typename T, typename... Args>
void append(std::ostringstream& out, std::vector<Tag>& tags, const T& var, Args... args)
{
    out << var;
    append(out, tags, args...);
}

template <typename... Args>
void append(std::ostringstream& out, std::vector<Tag>& tags, const Tag tag, Args... args)
{
    tags.emplace_back(tag);
    append(out, tags, args...);
}

void append(std::ostringstream&    , std::vector<Tag>&)
{
}

} // namespace detail

//
// Log function
//
template <typename... Args>
void log(Severity severity, const char* file, int line, Args... args) noexcept
{
    try {
        std::ostringstream msg;
        std::vector<Tag> tags;
        detail::append(msg, tags, args...);

        Entry e(std::chrono::system_clock::now(),
                file, line, severity, std::move(tags), msg.str());

        Relay::get().addEntry(std::move(e));
    } catch (...) {
        // TODO
        return;
    }
}


} // namespace logging

#endif // LOG_LOG_H
