#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include <chrono>
#include <string>
#include <vector>

#include "enums.h"

namespace logging
{

struct Entry
{
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;

    Entry() : filename{nullptr},
              line{0},
              severity{Severity::Ignore} {}


    Entry(std::chrono::system_clock::time_point&& t,
          const char* const f,
          int l,
          const Severity s,
          std::vector<Tag>&& tgs,
          std::string&& m
    )
      : timestamp{std::move(t)},
        filename{f},
        line{l},
        severity{s},
        tags{std::move(tgs)},
        msg{std::move(m)}
    {
    }

    Entry(Entry&& other) noexcept
      : timestamp{std::move(other.timestamp)},
        filename{other.filename},
        line{other.line},
        severity{other.severity},
        tags{std::move(other.tags)},
        msg{std::move(other.msg)}
    {
        other.severity = Severity::Ignore;
    }

    Entry& operator=(Entry&& other) noexcept
    {
        timestamp = other.timestamp;
        filename = other.filename;
        line = other.line;
        severity = other.severity;
        tags.swap(other.tags);
        msg.swap(other.msg);
        return *this;
    }

    std::chrono::system_clock::time_point       timestamp;
    const char*                                 filename;
    int                                         line;
    Severity                                    severity;
    std::vector<Tag>                            tags;
    std::string                                 msg;
};

} // namespace logging

#endif // LOG_ENTRY_H
