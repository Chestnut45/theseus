//-----------------------------------------------------------------------------
// File: StatusManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manager for StatusComponent.
//-----------------------------------------------------------------------------

#include "StatusManager.h"

StatusManager::StatusManager(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

StatusManager::~StatusManager()
{

}

void StatusManager::Update(float p_delta)
{
    for (auto&&[_,status] : this->m_scene->Each<StatusComponent>())
    {
        for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
        {
            StatusComponent::StatusEffect * statusEffect = status.m_aStatusEffects[i];
            
            if(statusEffect != nullptr)
            {
                if(statusEffect->GetTimer()->Elapsed() >= statusEffect->GetLifespan())
                {
                    std::cout << "StatusManager - Delete status effect: " << statusEffect->GetStatusEffectType() << std::endl;
                    
                    status.RemoveStatusEffect(statusEffect->GetStatusEffectType());
                }
                else
                {
                    statusEffect->ApplyStatusEffect();
                }
            }
        }
    }
}