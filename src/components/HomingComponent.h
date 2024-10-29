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
    HomingComponent(wolf::GameObject * p_target, float p_turning_cap, int p_delay = 0);
    virtual ~HomingComponent();

    void Update(float p_delta);
private:
    wolf::GameObject* m_pTarget = nullptr;
    int m_iUpdateDelay = 0;
    int m_iUpdateDelayCounter = 0;
    float m_fTurningCapRad = 0.0f;
};