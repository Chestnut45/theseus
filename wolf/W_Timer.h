//-----------------------------------------------------------------------------
// File:			W_Timer.h
// Original Author:	Youssef Ashraf
//
//
// A class that's responsible for the Engine's Timer.
// nice to have "threading"
//-----------------------------------------------------------------------------

#ifndef W_TIMER_H
#define W_TIMER_H

#include <chrono>
#include <entt/entt.hpp>

namespace wolf {

/// @brief A timer class that uses `std::chrono` for high-resolution timing
class Timer
{
public:
    

    Timer();
    
    void Start();
    void Stop();
    void Reset();
    bool IsRunning() const;

    /// @return The elapsed duration in milliseconds.
    bool HasElapsed() const;

private:
    
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::milliseconds;
    /// Type alias for the time point, which is a specific point in time
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint m_startTime;

    float m_duration{0.0f};
    bool m_isRunning{false};
    bool m_hasElapsed{false};
};

} 

#endif // W_TIMER_H