#include <algorithm>

#include "relay_impl.h"
#include "sink.h"

namespace logging
{

///////////////////////////////////////////////////////////////////////////

RelayImpl::RelayImpl()
noexcept
{
}

///////////////////////////////////////////////////////////////////////////

RelayImpl::~RelayImpl()
noexcept
{
    flush();
}

///////////////////////////////////////////////////////////////////////////

bool RelayImpl::addEntry(Entry&& entry)
noexcept
{
    try {
        bool result = true;
        for (auto& sink : m_sinks)
            result &= sink->addEntry(entry);
        return result;
    } catch (...) {
        // TODO
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////

void RelayImpl::flush()
noexcept
{
    for (auto& sink : m_sinks) {
        sink->flush();
    }
}

///////////////////////////////////////////////////////////////////////////

bool RelayImpl::registerSink(std::shared_ptr<Sink>& sink)
{
    const auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end())
        return false; // Already registered
    m_sinks.emplace_back(sink);
    return true;
}

///////////////////////////////////////////////////////////////////////////

bool RelayImpl::unregisterSink(const std::shared_ptr<Sink>& sink)
{
    auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end()) {
        m_sinks.erase(it);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////


} // namespace logging
