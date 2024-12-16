//-----------------------------------------------------------------------------
// File: HealthComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------

#include "HealthComponent.h"
#include "PlayerInventoryComponent.h"
#include "../inventory/ArmourItem.h"

// Constructor for custom health
HealthComponent::HealthComponent(int p_health)
{
    this->m_health = p_health;
    this->m_cap = p_health;
    this->m_vDamageIndicators;

    // Add Listeners for the healing events related to items
    wolf::EventManager::AddListener<PercentHealthItemEvent, HealthComponent, &HealthComponent::HandlePercentHealthItemEvent>(*this);
    wolf::EventManager::AddListener<FlatHealthItemEvent, HealthComponent, &HealthComponent::HandleFlatHealthItemEvent>(*this);
}

// Destructor
HealthComponent::~HealthComponent()
{
    // Remove the healing item event Listeners
    wolf::EventManager::RemoveListener<PercentHealthItemEvent, HealthComponent, &HealthComponent::HandlePercentHealthItemEvent>(*this);
    wolf::EventManager::RemoveListener<FlatHealthItemEvent, HealthComponent, &HealthComponent::HandleFlatHealthItemEvent>(*this);
}

void HealthComponent::Init()
{
}

// Get health
float HealthComponent::GetHealth() const
{
    return this->m_health;
}

// Reduce health
void HealthComponent::Damage(float p_damage)
{
    if(this->m_health > 0)
    {
        float damageReduction = 0.0f;

        // Get armour for damage reduction
        PlayerInventoryComponent* pic = this->GetGameObject()->GetComponent<PlayerInventoryComponent>();
        if(pic != nullptr)
        {
            ArmourItem* headgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::HEAD));
            damageReduction += headgear != nullptr ? headgear->GetDamageReduction() : 0;

            ArmourItem* bodygear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::BODY));
            damageReduction += bodygear != nullptr ? bodygear->GetDamageReduction() : 0;

            ArmourItem* armsgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::ARMS));
            damageReduction += armsgear != nullptr ? armsgear->GetDamageReduction() : 0;

            ArmourItem* glovesgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::GLOVES));
            damageReduction += glovesgear != nullptr ? glovesgear->GetDamageReduction() : 0;

            ArmourItem* legsgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::LEGS));
            damageReduction += legsgear != nullptr ? legsgear->GetDamageReduction() : 0;

            ArmourItem* feetgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::FEET));
            damageReduction += feetgear != nullptr ? feetgear->GetDamageReduction() : 0;

            ArmourItem* accessorygear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::ACCESSORY));
            damageReduction += accessorygear != nullptr ? accessorygear->GetDamageReduction() : 0;

            // std::cout << "HealthComponent - damred: " << damageReduction << std::endl;
        }

        float finalDamage = p_damage * (1.0f - damageReduction);
        this->m_health -= finalDamage;
        if (m_health < 0) m_health = 0;
        this->AddDamageIndicator(finalDamage);
    }
}

// Reduce health & ignore armour
void HealthComponent::Pierce(float p_damage)
{
    if(this->m_health > 0)
    {
        this->m_health -= p_damage;
        if (m_health < 0) m_health = 0;
        this->AddDamageIndicator(p_damage);
    }
}

// Increase health (with cap)
void HealthComponent::Heal(float p_heal)
{
    this->m_health += p_heal;
    if(this->m_health > this->m_cap)
    {
        this->m_health = this->m_cap;
    }
}

// Increase cap & refill health
void HealthComponent::Supercharge(float p_supercharge)
{
    this->m_cap += p_supercharge;
    this->m_health = this->m_cap;
}

void HealthComponent::UpdateDamageIndicators(float p_delta)
{
    if(this->m_vDamageIndicators.size() > 0)
    {
        // printf("HealthComponent - Update\n");
        std::vector<DamageIndicator>::iterator itr = this->m_vDamageIndicators.begin();
        while(itr != this->m_vDamageIndicators.end())
        {
            DamageIndicator* damageIndicator = &(*itr);

            // If lifetime expired, remove indicator
            if(damageIndicator->lifetime <= 0.0f)
            {
                this->m_vDamageIndicators.erase(itr);
            }
            else
            {
                damageIndicator->Update(p_delta);
                itr++;
            }
        }
    }
}

void HealthComponent::RenderDamageIndicators()
{
    if(this->m_vDamageIndicators.size() > 0)
    {        
        // Render damage indicators
        for(DamageIndicator damageIndicator: this->m_vDamageIndicators)
        {
            //damageIndicator.Render();
        }
    }    
}

void HealthComponent::AddDamageIndicator(float p_damage)
{
    this->m_vDamageIndicators.emplace_back(DamageIndicator{});
    int pos = this->m_vDamageIndicators.size() - 1;

    this->m_vDamageIndicators.at(pos).damageValue = std::to_string(p_damage);
    this->m_vDamageIndicators.at(pos).ownerComponent = this;
    this->m_vDamageIndicators.at(pos).currentPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    this->m_vDamageIndicators.at(pos).lifetime = 1.0f;
}

float HealthComponent::GetMaxHealth() const
{
    return m_cap;  
}

// !-- Aurora added these events --!
void HealthComponent::HandlePercentHealthItemEvent(const PercentHealthItemEvent& p_event) {
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt * m_cap);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt * m_cap);
    }
}

void HealthComponent::HandleFlatHealthItemEvent(const FlatHealthItemEvent& p_event) {
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt);
    }
}

void HealthComponent::DamageIndicator::Update(float p_delta)
{
    lifetime -= p_delta;
    // std::cout << "HealthComponent - Indicator Lifetime: " << lifetime << std::endl;
    if(ownerComponent != nullptr)
    {                
        
        currentPos.y += p_delta * 10.0f; // Indicator floats upwards
    }
}

void HealthComponent::DamageIndicator::Render()
{
    wolf::Scene* scene = &ownerComponent->GetGameObject()->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    glm::vec2 viewSize = camera->GetViewSize();
    glm::vec2 worldpos = currentPos;
    
    float l, r, t, b;
    l = camera->GetPosition().x - viewSize.x * 0.5f;
    r = camera->GetPosition().x + viewSize.x * 0.5f;
    t = camera->GetPosition().y + viewSize.y * 0.5f;
    b = camera->GetPosition().y - viewSize.y * 0.5f;
    
    // std::cout << "HealthComponent - Camerapos - x: " << camera->GetPosition().x << ", y: " << camera->GetPosition().y << std::endl;
    // std::cout << "HealthComponent - Indicatorpos - x: " << worldpos.x << ", y: " << worldpos.y << std::endl;
    // Check if indicator is visible
    if
    (
        worldpos.x >= l &&
        worldpos.x <= r &&
        worldpos.y <= t &&
        worldpos.y >= b
    )
    {   
        glm::vec2 screenpos;
        screenpos.x = (worldpos.x - (camera->GetPosition().x - viewSize.x * 0.5f));
        screenpos.y = (worldpos.y - (camera->GetPosition().y - viewSize.y * 0.5f)) * (-1) + viewSize.y;
        
        // Setup
        ImVec2 windowSize = ImVec2(640, 320);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;
        ImGui::SetNextWindowPos({80, 80});
        ImGui::SetNextWindowSize(windowSize);
        ImGui::Begin("\t", nullptr, flags);
        // std::cout << "HealthComponent - Screenpos - x: " << screenpos.x << ", y: " << screenpos.y << std::endl;
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f),  damageValue.c_str());

        // End rendering
        ImGui::End();
    }

}