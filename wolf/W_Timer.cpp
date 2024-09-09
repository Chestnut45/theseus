//-----------------------------------------------------------------------------
// File:			W_Timer.h
// Original Author:	Youssef Ashraf
//
//
// A class that's responsible for high resolution timing.
//-----------------------------------------------------------------------------
#include "W_Timer.h"

namespace wolf {

Timer::Timer()
    : m_isRunning(false)
{
    m_startTime = Clock::now();
    m_stopTime = m_startTime;
}

void Timer::Start()
{
    if (!m_isRunning)
    {
        m_startTime = Clock::now();
        m_isRunning = true;
    }
}

void Timer::Stop()
{
    if (m_isRunning)
    {
        m_stopTime = Clock::now();
        m_isRunning = false;
    }
}

void Timer::Reset()
{
    m_startTime = Clock::now();
    m_stopTime = m_startTime;
    m_isRunning = false;
}

void Timer::Restart()
{
    m_startTime = Clock::now();
    m_stopTime = m_startTime;
    m_isRunning = true;
}

bool Timer::IsRunning() const
{
    return m_isRunning;
}

double Timer::Elapsed() const
{
    TimePoint end = m_isRunning ? Clock::now() : m_stopTime;
    Duration elapsed = std::chrono::duration_cast<Duration>(end - m_startTime);
    return (double)elapsed.count() / 1000;
}


} // namespace wolf