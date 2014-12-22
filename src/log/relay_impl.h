#ifndef LOG_RELAY_IMPL_H
#define LOG_RELAY_IMPL_H

#include <vector>
#include <memory>

namespace logging
{

// Forward declarations
class Entry;
class Sink;


class RelayImpl
{
public:
    RelayImpl(const RelayImpl&) = delete;
    RelayImpl& operator=(const RelayImpl&) = delete;

    bool addEntry(Entry&& e) noexcept;

    bool registerSink(std::shared_ptr<Sink>& sink);

    bool unregisterSink(const std::shared_ptr<Sink>& sink);

    void flush() noexcept;

private:
    friend class Relay;
    RelayImpl() noexcept;
    ~RelayImpl() noexcept;

    std::vector<std::shared_ptr<Sink>>  m_sinks;
};


} // namespace logging

#endif // LOG_RELAY_IMPL_H
