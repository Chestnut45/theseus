#pragma once 
//-----------------------------------------------------------------------------
// File: HealthComponent.h
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------

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
    bool IsActive() const { return m_active; }
    
    void SetActive(bool p_active) { m_active = p_active; }
    void Damage(float p_damage);
    void Pierce(float p_damage);
    void Heal(float p_heal);
    void Supercharge(float p_supercharge);
    void GodmodeHeal(); // Only for use when in godmode

    void UpdateDamageIndicators(float p_delta);
    void RenderDamageIndicators();

    void HandlePercentHealthItemEvent(const PercentHealthItemEvent& p_event);
    void HandleFlatHealthItemEvent(const FlatHealthItemEvent& p_event);

private:
    float m_health = 100;
    float m_cap = 100;
    bool m_active = true;

    struct DamageIndicator
    {
        std::string id = ""; // For rendering ImGui window
        float lifetime = 0.5f;
        glm::vec2 currentPos = glm::vec2(0.0f, 0.0f);
        HealthComponent* ownerComponent = nullptr;
        std::string damageValue = "0";
        ImVec2 damageValueTextSize = ImVec2(0.0f, 0.0f);
        ImVec4 damageValueTextColour = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        static const inline ImVec2 WINDOW_SIZE = ImVec2(32, 16);

        DamageIndicator(){ idGenerator++; };
        void Update(float p_delta);
        void Render();

        static inline long long idGenerator = 0;
    };

    std::vector<DamageIndicator> m_vDamageIndicators;

    wolf::RNG m_RNG;

    void AddDamageIndicator(float p_damage, ImVec4 p_text_colour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void AddDamageIndicator(std::string p_damage_str, ImVec4 p_text_colour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
};