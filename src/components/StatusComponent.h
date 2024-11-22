//-----------------------------------------------------------------------------
// File: StatusComponent.h
// Original Author: Nguyễn Minh Nhật
// Status.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>


// #include "../ColliderManager.h"
#include "../events/InventoryEvents.h"

class StatusComponent : public wolf::BaseComponent
{
public:
    enum StatusEffectType
    {
        BURNING,
        PETRIFIED,
        POISONED,
        NONE
    };

    StatusComponent();
    virtual ~StatusComponent();

    void AddStatusEffect(StatusEffectType p_se_type, float p_lifespan);
    bool IsStatusEffectActive(StatusEffectType p_se_type) const;

    void Update(float p_delta);
    void RenderPlayerSEIcons();

private:
    struct StatusEffect
    {
        void ApplyStatusEffect(float p_delta);

        bool m_isActive = false;
        float m_fLifespan = 1.0f;
        StatusEffectType m_StatusEffectType;
        wolf::Timer m_timer;
        StatusComponent* m_OwnerComponent = nullptr;
    };

    StatusEffect m_aStatusEffects [StatusEffectType::NONE];

    static int s_iComponentCounter;
    
    static ImVec2 s_vTextureSize;
    static std::string s_aStatusEffectDescriptions[StatusEffectType::NONE];
    static wolf::Texture* s_pTextures[StatusEffectType::NONE];

    void RemoveStatusEffect(StatusEffectType p_se_type);
    
    // !-- Aurora added this method to be used with StatusEffectItems --!
    void HandleApplyStatusEffectEvent(const ApplyStatusEffectEvent& p_event);
};