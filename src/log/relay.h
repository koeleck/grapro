#ifndef PKOMMON_LOG_RELAY_HPP
#define PKOMMON_LOG_RELAY_HPP



#include <memory>
#include "enums.h"

namespace logging
{
///////////////////////////////////////////////////////////////////////////

// Forward declarations
class RelayImpl;
class RelayThreadsafeImpl;
class Sink;
class Entry;

///////////////////////////////////////////////////////////////////////////

class Relay
{
public:
    Relay(const Relay&) = delete;
    Relay& operator=(const Relay&) = delete;

    static bool initialize();
    static bool shutdown();

    static Relay& get() noexcept;

    bool addEntry(Entry&& e) noexcept;

    void flush() noexcept;

    bool registerSink(std::shared_ptr<Sink>& sink);

    bool unregisterSink(std::shared_ptr<Sink>& sink);

private:
    using Impl = RelayThreadsafeImpl;

    Relay(std::unique_ptr<Impl>&& impl) noexcept;

    std::unique_ptr<Impl>  m_impl;
};


} // namespace logging

#endif // PKOMMON_LOG_RELAY_HPP
