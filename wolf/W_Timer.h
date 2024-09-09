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
/// and `entt::delegate` for handling callbacks when the timer stops.
class Timer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::milliseconds;

    /// Type alias for the time point, which is a specific point in time
    using TimePoint = std::chrono::time_point<Clock>;

    Timer();
    
    void Start();
    void Stop();
    void Reset();
    bool IsRunning() const;

    /// @return The elapsed duration in milliseconds.
    Duration Elapsed() const;

    /// @brief Sets a callback function to be called when the timer stops.
    /// @tparam Args The types of the arguments that the callback function accepts.
    /// @param callback A pointer to the function to be called when the timer stops.
    /// @param args The arguments to be passed to the callback function.

    template<typename... Args>
    void SetCallback(void(*callback)(Args...), Args... args);

private:
    TimePoint m_startTime;
    bool m_isRunning;
    entt::delegate<void()> m_callback;
};

} 

#endif // W_TIMER_H