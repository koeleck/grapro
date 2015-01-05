#include "timer.h"

namespace core
{

/****************************************************************************/

Timer::Timer() = default;
Timer::~Timer() = default;

/****************************************************************************/

/*****************************************************************************
 *
 * CPUTimer
 *
 ****************************************************************************/

CPUTimer::CPUTimer()
  : m_start{clock::now()},
    m_running{false},
    m_time{0}
{
}

/****************************************************************************/

void CPUTimer::start()
{
    m_start = clock::now();
    m_running = true;
}

/****************************************************************************/

void CPUTimer::stop()
{
    m_time = std::chrono::duration_cast<duration>(clock::now() - m_start);
    m_running = false;
}

/****************************************************************************/

bool CPUTimer::isRunning() const
{
    return m_running;
}

/****************************************************************************/

CPUTimer::duration CPUTimer::time() const
{
    if (m_running) {
        return std::chrono::duration_cast<duration>(clock::now() - m_start);
    }
    return m_time;
}


/*****************************************************************************
 *
 * GPUTimer
 *
 ****************************************************************************/

GPUTimer::GPUTimer()
  : m_running{false},
    m_result_fetched{true},
    m_time{0}
{
    glQueryCounter(m_timer[0], GL_TIMESTAMP);
}

/****************************************************************************/

void GPUTimer::start()
{
    glQueryCounter(m_timer[0], GL_TIMESTAMP);
    m_running = true;
}

/****************************************************************************/

void GPUTimer::stop()
{
    glQueryCounter(m_timer[1], GL_TIMESTAMP);
    m_running = false;
    m_result_fetched = false;
}

/****************************************************************************/

bool GPUTimer::isRunning() const
{
    return m_running;
}

/****************************************************************************/

GPUTimer::duration GPUTimer::time() const
{
    if (m_running) {
        glQueryCounter(m_timer[1], GL_TIMESTAMP);
        return getTime();
    }
    if (!m_result_fetched) {
        m_time = getTime();
        m_result_fetched = true;
    }
    return m_time;
}

/****************************************************************************/

GPUTimer::duration GPUTimer::getTime() const
{
    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) {
        glGetQueryObjectiv(m_timer[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
    }

    GLuint64 startTime, stopTime;
    glGetQueryObjectui64v(m_timer[0], GL_QUERY_RESULT, &startTime);
    glGetQueryObjectui64v(m_timer[1], GL_QUERY_RESULT, &stopTime);

    return duration(stopTime - startTime); // nanoseconds
}

/****************************************************************************/

} // namespace core

