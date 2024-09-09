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
        m_hasElapsed = true;
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


} // namespace wolf