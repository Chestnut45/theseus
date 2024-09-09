//-----------------------------------------------------------------------------
// File:			W_Timer.h
// Original Author:	Youssef Ashraf
//
//
// A class that's responsible for the Engine's Timer.
//-----------------------------------------------------------------------------
#include "W_Timer.h"

namespace wolf {

Timer::Timer() : m_isRunning(false) {}

void Timer::Start()
{
    if (!m_isRunning) {
        m_startTime = Clock::now();
        m_isRunning = true;
    }
}

void Timer::Stop()
{
    if (m_isRunning) {
        m_isRunning = false;
        if (m_callback) {
            m_callback();
        }
    }
}

void Timer::Reset()
{
    m_isRunning = false;
    m_startTime = TimePoint();
}

bool Timer::IsRunning() const
{
    return m_isRunning;
}

Timer::Duration Timer::Elapsed() const
{
    if (m_isRunning) {
        return std::chrono::duration_cast<Duration>(Clock::now() - m_startTime);
    }
    return Duration::zero();
}

template<typename... Args>
void Timer::SetCallback(void(*callback)(Args...), Args... args)
{
    m_callback.connect<Args...>(callback, args...);
}

} // namespace wolf