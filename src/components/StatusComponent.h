#pragma once

//-----------------------------------------------------------------------------
// File: StatusComponent.h
// Original Author: Nguyễn Minh Nhật
// Applies status effects onto player/enemies/NPCs.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------

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
        HEALING,
        PETRIFIED,
        POISONED,
        NONE
    };

    StatusComponent();
    virtual ~StatusComponent();

    // Delete copy constructor/assignment
    StatusComponent(const StatusComponent&) = delete;
    StatusComponent& operator=(const StatusComponent&) = delete;

    // Delete move constructor/assignment
    StatusComponent(StatusComponent&& other) = delete;
    StatusComponent& operator=(StatusComponent&& other) = delete;

    void AddStatusEffect(StatusEffectType p_se_type, float p_lifespan);
    void SetStatusEffectResistance(StatusEffectType p_se_type, float p_resistance_value);
    bool IsStatusEffectActive(StatusEffectType p_se_type) const;
    float GetStatusEffectResistance(StatusEffectType p_se_type) const;

    void Update(float p_delta);
    void RenderPlayerSEIcons();

private:
    struct StatusEffect
    {
        void ApplyStatusEffect(float p_delta);

        bool m_isActive = false;
        float m_fLifespan = 1.0f;
        static const inline float SE_APPLICATION_INTERVALS [StatusEffectType::NONE] = {0.5f, 0.5f, 0.0f, 0.5f}; // Delay interval between instances of status effect application for each status effect

        float m_fSEApplicationTimer = 0.0f;
        StatusEffectType m_StatusEffectType;
        wolf::Timer m_timer; // Lifespan timer
        StatusComponent* m_OwnerComponent = nullptr;
    };

    StatusEffect m_aStatusEffects [StatusEffectType::NONE];     // Position in array corresponds to position of status effect in enum
    float m_aStatusEffectResistance [StatusEffectType::NONE];    // Resistance to status effect values

    static int s_iComponentCounter;
    
    static ImVec2 s_vTextureSize;
    static std::string s_aStatusEffectDescriptions[StatusEffectType::NONE];
    static wolf::Texture* s_pTextures[StatusEffectType::NONE];

    void RemoveStatusEffect(StatusEffectType p_se_type);
    
    // !-- Aurora added this method to be used with StatusEffectItems --!
    void HandleApplyStatusEffectEvent(const ApplyStatusEffectEvent& p_event);
};

struct StatusEffectAdditionEvent
{
    StatusComponent::StatusEffectType statusEffect = StatusComponent::StatusEffectType::NONE;
    float duration = 0.0f;
    wolf::GameObject* owner = nullptr;

    StatusEffectAdditionEvent(const StatusComponent::StatusEffectType p_se, const float p_duration, wolf::GameObject* p_owner) : statusEffect(p_se), duration(p_duration), owner(p_owner){}
};
