//-----------------------------------------------------------------------------
// File: HomingComponent.h
// Original Author: Nguyễn Minh Nhật
// Sets VelocityComponent towards target
//-----------------------------------------------------------------------------

#include <wolf.h>
#include <math.h>

class HomingComponent : public wolf::BaseComponent
{
public:
    HomingComponent(wolf::GameObject * p_target, float p_turning_cap, float p_delay = 0);
    virtual ~HomingComponent();

    void Update(float p_delta);
private:
    wolf::GameObject* m_pTarget = nullptr;
    float m_iUpdateDelay = 0.0f;
    float m_iUpdateDelayCounter = 0.0f;
    float m_fTurningCapRad = 0.0f;
};