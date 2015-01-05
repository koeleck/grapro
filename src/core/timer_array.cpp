#include "timer_array.h"

namespace core
{

/****************************************************************************/

TimerArray::TimerArray() = default;

/****************************************************************************/

TimerArray::~TimerArray() = default;

/****************************************************************************/

GPUTimer* TimerArray::addGPUTimer(const std::string& name)
{
    m_timers.emplace_back(std::make_pair(name,
                std::unique_ptr<Timer>(new GPUTimer())));
    return static_cast<GPUTimer*>(m_timers.back().second.get());
}

/****************************************************************************/

CPUTimer* TimerArray::addCPUTimer(const std::string& name)
{
    m_timers.emplace_back(std::make_pair(name,
                std::unique_ptr<Timer>(new CPUTimer())));
    return static_cast<CPUTimer*>(m_timers.back().second.get());
}

/****************************************************************************/

const TimerArray::TimerVector& TimerArray::getTimers() const
{
    return m_timers;
}

/****************************************************************************/

} // namespace core

