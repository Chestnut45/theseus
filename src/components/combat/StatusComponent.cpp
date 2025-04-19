//-----------------------------------------------------------------------------
// File: StatusComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Applies status effects onto player/enemies/NPCs.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------

#include "StatusComponent.h"

#include "PlayerController.h"

int StatusComponent::s_iComponentCounter = 0;
wolf::Texture* StatusComponent::s_pTextures[StatusComponent::StatusEffectType::NONE];
std::string StatusComponent::s_aStatusEffectDescriptions[StatusEffectType::NONE];
ImVec2 StatusComponent::s_vTextureSize = ImVec2(48.0f, 48.0f);

StatusComponent::StatusComponent()
{
    // Initialise status effect icon textures & descriptions
    if(s_iComponentCounter == 0)
    {
        s_pTextures[StatusComponent::StatusEffectType::BURNING] = wolf::TextureManager::CreateTexture("data/textures/SEBurning.png");
        s_pTextures[StatusComponent::StatusEffectType::BURNING]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        s_pTextures[StatusComponent::StatusEffectType::HEALING] = wolf::TextureManager::CreateTexture("data/textures/SEHealing.png");
        s_pTextures[StatusComponent::StatusEffectType::HEALING]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        s_pTextures[StatusComponent::StatusEffectType::PETRIFIED] = wolf::TextureManager::CreateTexture("data/textures/SEPetrified.png");
        s_pTextures[StatusComponent::StatusEffectType::PETRIFIED]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        s_pTextures[StatusComponent::StatusEffectType::POISONED] = wolf::TextureManager::CreateTexture("data/textures/SEPoisoned.png");
        s_pTextures[StatusComponent::StatusEffectType::POISONED]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);

        s_aStatusEffectDescriptions[StatusEffectType::BURNING] = "You Are Burning!";
        s_aStatusEffectDescriptions[StatusEffectType::HEALING] = "You Are Healing!";
        s_aStatusEffectDescriptions[StatusEffectType::PETRIFIED] = "You Are Petrified!";
        s_aStatusEffectDescriptions[StatusEffectType::POISONED] = "You Are Poisoned!";
    }
    s_iComponentCounter++;

    for(int i = 0; i < StatusEffectType::NONE; i++)
    {
        // Initialise variables
        this->m_aStatusEffects[i].m_OwnerComponent = this;
        this->m_aStatusEffects[i].m_StatusEffectType = (StatusEffectType)i;

        // Initialise all resistance values to 0
        this->m_aStatusEffectResistance[i] = 0.0f;
    }

    wolf::EventManager::AddListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

StatusComponent::~StatusComponent()
{
    wolf::EventManager::RemoveListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
    s_iComponentCounter--;
    if(s_iComponentCounter == 0)
    {
        for (int i = 0; i < StatusEffectType::NONE; i++)
        {
            wolf::TextureManager::DestroyTexture(s_pTextures[i]);
            s_pTextures[i] = nullptr;
        }
    }
}


void StatusComponent::AddStatusEffect(StatusEffectType p_se_type, float p_lifespan)
{
    this->m_aStatusEffects[p_se_type].m_isActive = true;
    this->m_aStatusEffects[p_se_type].m_timer.Restart();
    this->m_aStatusEffects[p_se_type].m_fLifespan = p_lifespan;
    
}

void StatusComponent::SetStatusEffectResistance(StatusEffectType p_se_type, float p_resistance_value)
{
    // If resistance value is negative, return
    if(p_resistance_value < 0.0f) return;

    float absoluteValue = p_resistance_value;
    float left, right; // left = integral, right = decimal
    
    right = std::modf(absoluteValue, &left); // Getting integral & decimal
    left = left <= 1.0f ? 0.0f : 1.0f;

    // If input value is equal to 1, set resistance value to 1
    // if input value is smaller or larger than 1, set resistance value to only the decimal part
    m_aStatusEffectResistance[p_se_type] = left == 1.0f ? left : right; 
}

bool StatusComponent::IsStatusEffectActive(StatusEffectType p_se_type) const
{
    return this->m_aStatusEffects[p_se_type].m_isActive;
}

void StatusComponent::Update(float p_delta)
{
    for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
    {
        StatusComponent::StatusEffect& statusEffect = this->m_aStatusEffects[i];
        
        // Apply status effect
        if(statusEffect.m_isActive)
        {   
            // Count down timer
            statusEffect.m_fSEApplicationTimer -= p_delta;
            
            // If application interval expired, deal damage & reset timer
            if(statusEffect.m_fSEApplicationTimer <= 0.0f)
            {            
                statusEffect.ApplyStatusEffect(p_delta);
                statusEffect.m_fSEApplicationTimer = StatusEffect::SE_APPLICATION_INTERVALS[statusEffect.m_StatusEffectType];
            }

            // If lifetime expired, remove status effect
            if(statusEffect.m_fLifespan >= 0 && statusEffect.m_timer.Elapsed() >= statusEffect.m_fLifespan)
            {
                this->RemoveStatusEffect(statusEffect.m_StatusEffectType);
            }
        }
    }
}

float StatusComponent::GetStatusEffectResistance(StatusEffectType p_se_type) const
{
    return this->m_aStatusEffectResistance[p_se_type];
}

void StatusComponent::RemoveStatusEffect(StatusEffectType p_se_type)
{
    // Set active flag & reset application timer
    this->m_aStatusEffects[p_se_type].m_isActive = false;
    this->m_aStatusEffects[p_se_type].m_fSEApplicationTimer = StatusEffect::SE_APPLICATION_INTERVALS[this->m_aStatusEffects[p_se_type].m_StatusEffectType];
}

void StatusComponent::RenderPlayerSEIcons()
{
    // Setup
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoBackground;
    ImVec2 windowSize = ImVec2((s_vTextureSize.x + 16) * (float)StatusEffectType::NONE + 8, s_vTextureSize.y + 24);
    ImGui::SetNextWindowPos({0, 74});
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("##SEIcons", nullptr, flags);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

    // Render icons
    int activeIcons = 0;
    for (int i = 0; i < StatusEffectType::NONE; i++)
    {
        StatusEffectType seType = static_cast<StatusEffectType>(i);
        if(this->IsStatusEffectActive(seType))
        {
            // Fix spacing...
            ImGui::SetCursorPosX(activeIcons * (s_vTextureSize.x + 8) + 8.0f);
            activeIcons++;

            if (ImGui::ImageButton(std::to_string(seType).c_str(), (void*)(intptr_t)s_pTextures[seType]->GetID(), s_vTextureSize)) {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) 
            {
                float lifetime = m_aStatusEffects[seType].m_timer.Elapsed();
                float lifespan = m_aStatusEffects[seType].m_fLifespan;
                ImGui::BeginTooltip();
                if(lifespan > lifetime) ImGui::Text("%s\n%.1f", s_aStatusEffectDescriptions[seType].c_str(), lifespan - lifetime);
                else ImGui::Text("%s\n%s", s_aStatusEffectDescriptions[seType].c_str(),"inf");
                ImGui::EndTooltip();
            }
        }
        ImGui::SameLine();
    }

    // End rendering
    ImGui::PopStyleColor(3);  
    ImGui::End();
}

void StatusComponent::StatusEffect::ApplyStatusEffect(float p_delta)
{
    switch (this->m_StatusEffectType)
    {
        // Deal 16 pierce damage (excluding resistance)
        case StatusEffectType::BURNING:
        {
            HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
            if(health != nullptr)
            {
                float resistance = m_OwnerComponent->m_aStatusEffectResistance[StatusEffectType::BURNING];
                health->Pierce(15.0f * (1.0f - resistance));
            }
            else
            {
                std::cout << "StatusComponent - ERROR: HealthComponent not found." << std::endl;
            }
            break;
        }

        // Heal 8 hp
        case StatusEffectType::HEALING:
        {        
            HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
            if(health != nullptr)
            {
                health->Heal(10.0f);
            }
            else
            {
                std::cout << "StatusComponent - ERROR: HealthComponent not found." << std::endl;
            }
            break;
        }

        // No effect
        case StatusEffectType::PETRIFIED:
        {
            break;
        }      
        
        // Deal 8 pierce damage (excluding resistance)
        case StatusEffectType::POISONED:
        {
            HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
            if(health != nullptr)
            {
                float resistance = m_OwnerComponent->m_aStatusEffectResistance[StatusEffectType::POISONED];
                health->Pierce(10.0f * (1.0f - resistance));
            }
            else
            {
                std::cout << "StatusComponent - ERROR: HealthComponent not found." << std::endl;
            }
            break;
        }
    }
}

// !-- Aurora added this method to be used with StatusEffectItems -- !
void StatusComponent::HandleApplyStatusEffectEvent(const ApplyStatusEffectEvent& p_event) {
    if (!GetGameObject()->HasAll<PlayerController>()) return;
    this->AddStatusEffect(static_cast<StatusComponent::StatusEffectType>(p_event.iType), p_event.fDuration);
}