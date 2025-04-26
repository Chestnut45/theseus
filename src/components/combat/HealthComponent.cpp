//-----------------------------------------------------------------------------
// File: HealthComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------

#include <W_EventManager.h>
#include <HealthComponent.h>
#include <PlayerInventoryComponent.h>
#include <ArmourItem.h>
#include <DamageEvent.h>
#include <PlayerController.h>

// Constructor for custom health
HealthComponent::HealthComponent(int p_health)
{
    this->m_health = p_health;
    this->m_cap = p_health;
    this->m_vDamageIndicators = {};

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

float HealthComponent::GetMaxHealth() const
{
    return m_cap;  
}

// Get health
float HealthComponent::GetHealth() const
{
    return this->m_health;
}

// Reduce health
void HealthComponent::Damage(float p_damage)
{
    if(!m_active) return;
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
        }

        float finalDamage = p_damage * (1.0f - damageReduction);
        this->m_health -= finalDamage;
        if (m_health < 0) m_health = 0;
        this->AddDamageIndicator(finalDamage, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // Send out a damage event
    DamageEvent event;
    event.m_damage = p_damage;
    event.m_pierce = false;
    event.m_pDamagedObject = GetGameObject();
    wolf::EventManager::TriggerEvent(event);
}

// Reduce health & ignore armour
void HealthComponent::Pierce(float p_damage)
{
    if(!m_active) return;
    
    if(this->m_health > 0)
    {
        this->m_health -= p_damage;
        if (m_health < 0) m_health = 0;
        this->AddDamageIndicator(p_damage, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

        // Send out a damage event
        DamageEvent event;
        event.m_damage = p_damage;
        event.m_pierce = true;
        event.m_pDamagedObject = GetGameObject();
        wolf::EventManager::TriggerEvent(event);
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
    this->AddDamageIndicator(std::string("+") + std::to_string((int)p_heal), ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
}

// Increase cap & refill health
void HealthComponent::Supercharge(float p_supercharge)
{
    this->m_cap += p_supercharge;
    this->m_health = this->m_cap;
    this->AddDamageIndicator(std::string("+") + std::to_string((int)this->m_cap), ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
}

// Only for use when in godmode
void HealthComponent::GodmodeHeal()
{
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
            // Else, update damage indicator
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
            damageIndicator.Render();
        }
    }    
}

void HealthComponent::AddDamageIndicator(float p_damage, ImVec4 p_text_colour)
{
    // Add a new damage indicator
    this->m_vDamageIndicators.emplace_back(DamageIndicator());
    int index = this->m_vDamageIndicators.size() - 1;

    // Get transform & damage value as a string
    wolf::Transform2D* gameobjTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    std::string damageValueString = std::to_string((int)p_damage);
    
    // Add data to the damage indicator
    DamageIndicator* dmg_ind = &this->m_vDamageIndicators.at(index);
    dmg_ind->id = std::to_string(DamageIndicator::idGenerator);
    dmg_ind->damageValue = damageValueString;
    dmg_ind->damageValueTextSize = ImGui::CalcTextSize(damageValueString.c_str());
    dmg_ind->damageValueTextColour = p_text_colour;
    dmg_ind->ownerComponent = this;
    dmg_ind->currentPos = 
                            gameobjTransform->GetGlobalPosition()                                                   +   // Set window position to position of gameobj
                            glm::vec2(-DamageIndicator::WINDOW_SIZE.x, DamageIndicator::WINDOW_SIZE.y) * 0.5f       +   // Centre window
                            glm::vec2(m_RNG.NextFloat(-8.0f, 8.0f) , 10.0f) * gameobjTransform->GetGlobalScale();       // Offset window
    dmg_ind->lifetime = 1.0f;
}

void HealthComponent::AddDamageIndicator(std::string p_damage_str, ImVec4 p_text_colour)
{
    // Add a new damage indicator
    this->m_vDamageIndicators.emplace_back(DamageIndicator());
    int index = this->m_vDamageIndicators.size() - 1;

    // Get transform & damage value as a string
    wolf::Transform2D* gameobjTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    std::string damageValueString = p_damage_str;

    // Add data to the damage indicator
    DamageIndicator* dmg_ind = &this->m_vDamageIndicators.at(index);
    dmg_ind->id = std::to_string(DamageIndicator::idGenerator);
    dmg_ind->damageValue = damageValueString;
    dmg_ind->damageValueTextSize = ImGui::CalcTextSize(damageValueString.c_str());
    dmg_ind->damageValueTextColour = p_text_colour;
    dmg_ind->ownerComponent = this;
    dmg_ind->currentPos = 
                            gameobjTransform->GetGlobalPosition()                                                   +   // Set window position to position of gameobj
                            glm::vec2(-DamageIndicator::WINDOW_SIZE.x, DamageIndicator::WINDOW_SIZE.y) * 0.5f       +   // Centre window
                            glm::vec2(m_RNG.NextFloat(-8.0f, 8.0f) , 10.0f) * gameobjTransform->GetGlobalScale();       // Offset window
    dmg_ind->lifetime = 1.0f;
}

// !-- Aurora added this event --!
// Handler for PercentHealthItemEvents that heals the HealthComponent by a given percentage of its total health
// > p_event: the PercentHealthItemEvent object
void HealthComponent::HandlePercentHealthItemEvent(const PercentHealthItemEvent& p_event) {
    if (!GetGameObject()->HasAll<PlayerController>()) return;
    wolf::Audio::Play("data/sounds/sfx_heal.wav", 0.5f);
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt * m_cap);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt * m_cap);
    }
}

// !-- Aurora added this event --!
// Handler for FlatHealthItemEvents that heals the HealthComponent by a given amount
// > p_event: the FlatHealthItemEvent object
void HealthComponent::HandleFlatHealthItemEvent(const FlatHealthItemEvent& p_event) {
    if (!GetGameObject()->HasAll<PlayerController>()) return;
    wolf::Audio::Play("data/sounds/sfx_heal.wav", 0.5f);
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt);
    }
}

void HealthComponent::DamageIndicator::Update(float p_delta)
{
    // Update lifetime
    lifetime -= p_delta;
    if(ownerComponent != nullptr)
    {                
        
        currentPos.y += p_delta * 10.0f; // Indicator floats upwards
    }
}

void HealthComponent::DamageIndicator::Render()
{
    // Get data for calculations
    wolf::Scene* scene = &ownerComponent->GetGameObject()->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    glm::vec2 worldpos = currentPos;
    
    // Convert world space into screen space
    glm::vec4 clipSpacePos = camera->GetMatrix() * glm::vec4(worldpos, 0.0f, 1.0f);
    glm::vec3 ndc = glm::vec3(clipSpacePos) / clipSpacePos.w;
    glm::vec2 screenpos;
    screenpos.x = (ndc.x * 0.5f + 0.5f) * camera->GetViewSize().x;
    screenpos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * camera->GetViewSize().y;
            
    // Setup
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground |
                        ImGuiWindowFlags_NoMouseInputs |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoFocusOnAppearing | // Prevent focus
                        ImGuiWindowFlags_NoBringToFrontOnFocus; // Prevent altering window order        
    ImGui::SetNextWindowPos({screenpos.x, screenpos.y});
    ImGui::SetNextWindowSize(DamageIndicator::WINDOW_SIZE);
    ImGui::Begin(id.c_str(), nullptr, flags);
    ImGui::SetCursorPosX((DamageIndicator::WINDOW_SIZE.x - damageValueTextSize.x) * 0.5f);
    ImGui::TextColored(damageValueTextColour, "%s", damageValue.c_str());

    // End rendering
    ImGui::End();

}