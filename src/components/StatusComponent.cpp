//-----------------------------------------------------------------------------
// File: StatusComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Status.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------

#include "StatusComponent.h"

StatusComponent::StatusComponent()
{
    for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
    {
        this->m_aStatusEffects[i] = nullptr;
        this->m_aStatusEffectsPresenceFlags[i] = 0;
    }
}

StatusComponent::~StatusComponent()
{
}

void StatusComponent::AddStatusEffect(StatusComponent::StatusEffectType p_se_type, float p_lifespan)
{

    if(this->m_aStatusEffectsPresenceFlags[p_se_type])
    {
        this->m_aStatusEffects[p_se_type]->GetTimer()->Reset();
    }
    else
    {
        this->m_aStatusEffectsPresenceFlags[p_se_type] = 1;
        StatusComponent::StatusEffect * statusEffect = new StatusComponent::StatusEffect(p_se_type, p_lifespan);
        this->m_aStatusEffects[p_se_type] = statusEffect;
    }
    
}

void StatusComponent::RemoveStatusEffect(StatusComponent::StatusEffectType p_se_type)
{
    delete this->m_aStatusEffects[p_se_type];
    this->m_aStatusEffects[p_se_type] = nullptr;
}