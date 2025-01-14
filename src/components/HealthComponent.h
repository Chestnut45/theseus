//-----------------------------------------------------------------------------
// File: HealthComponent.h
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------
#pragma once 

#include <glm/glm.hpp>
#include <wolf.h>

#include "../events/InventoryEvents.h"

class HealthComponent : public wolf::BaseComponent
{
public:
    HealthComponent(int p_health);
    ~HealthComponent();

    // Delete copy constructor/assignment
    HealthComponent(const HealthComponent&) = delete;
    HealthComponent& operator=(const HealthComponent&) = delete;

    // Delete move constructor/assignment
    HealthComponent(HealthComponent&& other) = delete;
    HealthComponent& operator=(HealthComponent&& other) = delete;
    
    void Init();
    float GetMaxHealth() const;

    float GetHealth() const;

    void Damage(float p_damage);
    void Pierce(float p_damage);
    void Heal(float p_heal);
    void Supercharge(float p_supercharge);

    void UpdateDamageIndicators(float p_delta);
    void RenderDamageIndicators();

    void HandlePercentHealthItemEvent(const PercentHealthItemEvent& p_event);
    void HandleFlatHealthItemEvent(const FlatHealthItemEvent& p_event);

private:
    float m_health = 100;
    float m_cap = 100;

    struct DamageIndicator
    {
        std::string damageValue = "0";
        float lifetime = 1.0f;
        glm::vec2 currentPos = glm::vec2(0.0f, 0.0f);
        HealthComponent* ownerComponent = nullptr;

        void Update(float p_delta);
        void Render();
    };

    std::vector<DamageIndicator> m_vDamageIndicators;

    void AddDamageIndicator(float p_damage);
};