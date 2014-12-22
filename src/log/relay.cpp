#include <cassert>

#include "relay.h"
#include "relay_impl.h"
#include "relay_threadsafe_impl.h"

namespace
{
std::unique_ptr<logging::Relay> the_relay;
}

namespace logging
{

///////////////////////////////////////////////////////////////////////////

bool Relay::initialize()
{
    if (the_relay)
        return false;

    std::unique_ptr<Impl> impl(new Impl());
    the_relay.reset(new Relay(std::move(impl)));

    return true;
}

///////////////////////////////////////////////////////////////////////////

bool Relay::shutdown()
{
    if (!the_relay)
        return false;

    the_relay.reset();
    return true;
}

///////////////////////////////////////////////////////////////////////////

Relay& Relay::get()
noexcept
{
    assert(the_relay);
    return *the_relay.get();
}

///////////////////////////////////////////////////////////////////////////

Relay::Relay(std::unique_ptr<Impl>&& impl)
noexcept
  : m_impl{std::move(impl)}
{
}

///////////////////////////////////////////////////////////////////////////

bool Relay::addEntry(Entry&& e)
noexcept
{
    return m_impl->addEntry(std::move(e));
}

///////////////////////////////////////////////////////////////////////////

bool Relay::registerSink(std::shared_ptr<Sink>& sink)
{
    return m_impl->registerSink(sink);
}

///////////////////////////////////////////////////////////////////////////

void Relay::flush() noexcept
{
    m_impl->flush();
}

///////////////////////////////////////////////////////////////////////////

bool Relay::unregisterSink(std::shared_ptr<Sink>& sink)
{
    return m_impl->unregisterSink(sink);
}

///////////////////////////////////////////////////////////////////////////

} // namespace logging
