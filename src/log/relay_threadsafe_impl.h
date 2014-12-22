#ifndef LOG_RELAY_THREADSAFE_IMPL_H
#define LOG_RELAY_THREADSAFE_IMPL_H

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <boost/circular_buffer.hpp>

#include "entry.h"

namespace logging
{

// Forward declarations
class Sink;


class RelayThreadsafeImpl
{
public:
    RelayThreadsafeImpl(const RelayThreadsafeImpl&) = delete;
    RelayThreadsafeImpl& operator=(const RelayThreadsafeImpl&) = delete;

    ~RelayThreadsafeImpl() noexcept;

    bool addEntry(Entry&& e) noexcept;

    bool registerSink(std::shared_ptr<Sink>& sink);

    bool unregisterSink(const std::shared_ptr<Sink>& sink);

    void flush() noexcept;

private:
    friend class Relay;
    RelayThreadsafeImpl() noexcept;

    void worker();
    void process_entry() noexcept;
    void flush_impl() noexcept;

    std::vector<std::shared_ptr<Sink>>  m_sinks;
    boost::circular_buffer<Entry>       m_queue;
    std::mutex                          m_lock;
    std::mutex                          m_sink_lock;
    std::condition_variable             m_cv;
    std::atomic_flag                    m_running;
    std::atomic_flag                    m_flush;
    std::thread                         m_worker;
};


} // namespace logging

#endif // LOG_RELAY_THREADSAFE_IMPL_H
