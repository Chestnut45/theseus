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
    for(int i = 0; i < StatusEffectType::NONE; i++)
    {
        this->m_aStatusEffects[i] = nullptr;
    }

    wolf::EventManager::AddListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

StatusComponent::~StatusComponent()
{
    wolf::EventManager::RemoveListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

void StatusComponent::AddStatusEffect(StatusEffectType p_se_type, float p_lifespan)
{

    if(this->m_aStatusEffects[p_se_type] != nullptr)
    {
        this->m_aStatusEffects[p_se_type]->GetTimer()->Reset();
    }
    else
    {
        StatusEffect* statusEffect = new StatusEffect(p_se_type, p_lifespan, this);
        this->m_aStatusEffects[p_se_type] = statusEffect;
    }
    
}

bool StatusComponent::IsStatusEffectTypePresent(StatusEffectType p_se_type) const
{
    return this->m_aStatusEffects[p_se_type] == nullptr;
}

void StatusComponent::RemoveStatusEffect(StatusEffectType p_se_type)
{
    delete this->m_aStatusEffects[p_se_type];
    this->m_aStatusEffects[p_se_type] = nullptr;
}

// !-- Aurora added this method to be used with StatusEffectItems -- !
void StatusComponent::HandleApplyStatusEffectEvent(const ApplyStatusEffectEvent& p_event) {
    this->AddStatusEffect(static_cast<StatusComponent::StatusEffectType>(p_event.iType), p_event.fDuration);
}