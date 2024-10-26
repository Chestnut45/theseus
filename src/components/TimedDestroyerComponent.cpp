//-----------------------------------------------------------------------------
// File: TimedDestroyerComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Destroys object after a period of time
//-----------------------------------------------------------------------------

#include "TimedDestroyerComponent.h"

TimedDestroyerComponent::TimedDestroyerComponent(float p_lifespan, bool p_is_frame_timer)
{
    this->m_fTimer = 0.0f;
    this->m_bIsFrameTimer = p_is_frame_timer;

    this->m_fLifespan = this->m_bIsFrameTimer ? floor(p_lifespan) : p_lifespan;
}

TimedDestroyerComponent::~TimedDestroyerComponent()
{
}

void TimedDestroyerComponent::Update(float p_delta)
{   
    if(this->m_fTimer >= m_fLifespan)
    {
        printf("TimedDestroyerComponent - Destroy\n");
        this->GetGameObject()->GetScene().DeleteObject(this->GetGameObject()->GetID());
    }
    this->m_fTimer = this->m_bIsFrameTimer ? this->m_fTimer + 1 : this->m_fTimer + p_delta;
}