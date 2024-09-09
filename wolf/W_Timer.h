//-----------------------------------------------------------------------------
// File:			W_Timer.h
// Original Author:	Youssef Ashraf
//
//
// A class that's responsible for high resolution timing.
//-----------------------------------------------------------------------------

#ifndef W_TIMER_H
#define W_TIMER_H

#include <chrono>

namespace wolf
{

/// @brief A timer class that uses `std::chrono` for high-resolution timing
class Timer
{
public:

    Timer();

    // Starts the timer if it is not already running
    void Start();

    // Stops the timer if it is running
    void Stop();

    // Resets elapsed time to 0 and stops the timer
    void Reset();

    // Resets elapsed time to 0 and restarts the timer
    void Restart();

    // Returns the number of seconds that have elapsed since the
    // timer started if it is currently running, or the number of
    // seconds between start and stop if the timer is not running.
    double Elapsed() const;

    // Returns true if the timer is currently running, false otherwise
    bool IsRunning() const;

private:
    
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::milliseconds;
    using TimePoint = std::chrono::time_point<Clock>;

    // Time points for the start and stop time
    TimePoint m_startTime;
    TimePoint m_stopTime;

    // Flag
    bool m_isRunning;
};

} 

#endif // W_TIMER_H