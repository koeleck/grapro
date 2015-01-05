#ifndef CORE_TIMER_ARRAY_H
#define CORE_TIMER_ARRAY_H

#include <vector>
#include <memory>
#include <utility>
#include <string>
#include "timer.h"

namespace core
{

class TimerArray
{
public:
    using TimerVector = std::vector<std::pair<std::string, std::unique_ptr<Timer>>>;

    TimerArray();
    ~TimerArray();

    GPUTimer* addGPUTimer(const std::string& name);
    CPUTimer* addCPUTimer(const std::string& name);

    const TimerVector& getTimers() const;

private:
    TimerVector         m_timers;
};

} // namespace core

#endif // CORE_TIMER_ARRAY_H

