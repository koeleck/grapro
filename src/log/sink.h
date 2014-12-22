#ifndef LOG_SINK_H
#define LOG_SINK_H

namespace logging
{

// Forward declarations:
class Entry;


class Sink
{
public:
    virtual ~Sink() = default;

    virtual bool addEntry(const Entry& entry) noexcept = 0;

    virtual void flush() noexcept {}
};


} // namespace logging

#endif // LOG_SINK_H
