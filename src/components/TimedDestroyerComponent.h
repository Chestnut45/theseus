//-----------------------------------------------------------------------------
// File: TimedDestroyerComponent.h
// Original Author: Nguyễn Minh Nhật
// Destroys object after a period of time
//-----------------------------------------------------------------------------

#include <wolf.h>

class TimedDestroyerComponent : public wolf::BaseComponent
{
public:
    TimedDestroyerComponent(float p_lifespan, bool p_is_frame_timer = false);
    virtual ~TimedDestroyerComponent();

    void Update(float p_delta);
private:
    float m_fLifespan = 0.0f;
    float m_fTimer = 0.0f;
    bool m_bIsFrameTimer;
};