//-----------------------------------------------------------------------------
// File:			W_Timer.h
// Original Author:	Youssef Ashraf
//
//
// A class that's responsible for high resolution timing.
//-----------------------------------------------------------------------------
#include "W_Timer.h"

#include "W_EventManager.h"

namespace wolf {

Timer::Timer()
    :
    m_isRunning(false),
    m_isPaused(false)
{
    m_startTime = Clock::now();
    m_stopTime = m_startTime;

    wolf::EventManager::AddListener<PauseEvent, Timer, &Timer::OnPauseEvent>(*this);
}

Timer::~Timer()
{
    wolf::EventManager::RemoveListener<PauseEvent, Timer, &Timer::OnPauseEvent>(*this);
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

void Timer::Pause()
{
    if (m_isRunning && !m_isPaused)
    {
        // At this point, elapsed stores how long the timer ran before being paused
        m_stopTime = Clock::now();
        m_isPaused = true;
    }
}

void Timer::Unpause()
{
    if (m_isRunning && m_isPaused)
    {
        // Adjust start time as if the pause never happened
        Duration elapsed = std::chrono::duration_cast<Duration>(m_stopTime - m_startTime);
        m_startTime = Clock::now() - elapsed;
        m_isPaused = false;
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
    TimePoint end = (m_isRunning && !m_isPaused) ? Clock::now() : m_stopTime;
    Duration elapsed = std::chrono::duration_cast<Duration>(end - m_startTime);
    return (double)elapsed.count() / 1000;
}

void Timer::OnPauseEvent(const PauseEvent& event)
{
    event.m_paused ? Pause() : Unpause();
}

} // namespace wolf