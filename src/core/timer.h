#ifndef CORE_TIMER_H
#define CORE_TIMER_H

#include <chrono>
#include "gl/gl_objects.h"

namespace core
{

/****************************************************************************/

class Timer
{
public:
    using duration = std::chrono::nanoseconds;

    Timer();
    virtual ~Timer();

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual duration time() const = 0;
};

/****************************************************************************/

class CPUTimer
  : public Timer
{
public:
    using clock = std::chrono::high_resolution_clock;

    CPUTimer();
    virtual void start() override final;
    virtual void stop() override final;
    virtual bool isRunning() const override final;
    virtual duration time() const override final;

private:
    clock::time_point       m_start;
    bool                    m_running;
    duration                m_time;
};

/****************************************************************************/

class GPUTimer
  : public Timer
{
public:
    GPUTimer();
    virtual void start() override final;
    virtual void stop() override final;
    virtual bool isRunning() const override final;
    virtual duration time() const override final;

private:
    duration getTime() const;

    gl::Query           m_timer[2];
    bool                m_running;
    mutable bool        m_result_fetched;
    mutable duration    m_time;
};


/****************************************************************************/

} // namespace core

#endif // CORE_TIMER_H

