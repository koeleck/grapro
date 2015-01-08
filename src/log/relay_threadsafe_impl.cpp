#include <algorithm>

#include "relay_threadsafe_impl.h"
#include "sink.h"

namespace logging
{

constexpr short k_maxQueueSize {100};

///////////////////////////////////////////////////////////////////////////

RelayThreadsafeImpl::RelayThreadsafeImpl() noexcept
  : m_running{},
    m_flush{false}
{
    m_running.test_and_set();
    m_flush.test_and_set();

    m_worker = std::thread(&RelayThreadsafeImpl::worker, this);
}

///////////////////////////////////////////////////////////////////////////

RelayThreadsafeImpl::~RelayThreadsafeImpl() noexcept
{
    m_flush.clear();
    m_running.clear();

    m_cv.notify_one();
    m_worker.join();
}

///////////////////////////////////////////////////////////////////////////

bool RelayThreadsafeImpl::addEntry(Entry&& entry) noexcept
{
    while (true) {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_queue.size() < k_maxQueueSize) {
            m_queue.push_back(std::move(entry));
            break;
        }
    }
    m_cv.notify_one();
    return true;
}

///////////////////////////////////////////////////////////////////////////

void RelayThreadsafeImpl::flush() noexcept
{
    m_flush.clear();
}

///////////////////////////////////////////////////////////////////////////

void RelayThreadsafeImpl::flush_impl() noexcept
{
    std::unique_lock<std::mutex> lock(m_sink_lock);

    for (auto& sink : m_sinks) {
        try {
            sink->flush();
        } catch (...) {
            // TODO
        }
    }
}

///////////////////////////////////////////////////////////////////////////

bool RelayThreadsafeImpl::registerSink(std::shared_ptr<Sink>& sink)
{
    std::unique_lock<std::mutex> lock(m_sink_lock);

    const auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end())
        return false; // Already registered
    m_sinks.emplace_back(sink);
    return true;
}

///////////////////////////////////////////////////////////////////////////

bool RelayThreadsafeImpl::unregisterSink(const std::shared_ptr<Sink>& sink)
{
    std::unique_lock<std::mutex> lock(m_sink_lock);

    auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end()) {
        m_sinks.erase(it);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////

void RelayThreadsafeImpl::worker()
{
    bool running = true;
    while (running || !m_queue.empty()) {
        if (!m_flush.test_and_set())
            flush_impl();

        {
            std::unique_lock<std::mutex> lock(m_lock);
            running = running && m_running.test_and_set();
            if (m_queue.empty() && running)
                m_cv.wait(lock);

            process_entry();
        }
    }
    flush_impl();
}

///////////////////////////////////////////////////////////////////////////

void RelayThreadsafeImpl::process_entry() noexcept
{
    if (m_queue.empty())
        return;
    Entry e = std::move(m_queue.front());
    m_queue.pop_front();
    const bool do_flush = e.severity == Severity::Error;
    {
        std::unique_lock<std::mutex> sink_lock(m_sink_lock);
        for (auto& s : m_sinks) {
            try {
                s->addEntry(e);
                if (do_flush)
                    s->flush();
            } catch (...) {
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////


} // namespace logging
