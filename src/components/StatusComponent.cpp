//-----------------------------------------------------------------------------
// File: StatusComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Status.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------

#include "StatusComponent.h"

#include "AnimatedSprite2D.h"
#include "PlayerController.h"
#include "VelocityComponent.h"

int StatusComponent::s_iComponentCounter = 0;
wolf::Texture* StatusComponent::s_pTextures[StatusComponent::StatusEffectType::NONE];
ImVec2 StatusComponent::s_vTextureSize = ImVec2(64.0f, 64.0f);

StatusComponent::StatusComponent()
{
    if(s_iComponentCounter == 0)
    {
        s_pTextures[StatusComponent::StatusEffectType::BURNING] = wolf::TextureManager::CreateTexture("data/textures/SEBurning.png");
        s_pTextures[StatusComponent::StatusEffectType::BURNING]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        s_pTextures[StatusComponent::StatusEffectType::PETRIFIED] = wolf::TextureManager::CreateTexture("data/textures/SEPetrified.png");
        s_pTextures[StatusComponent::StatusEffectType::PETRIFIED]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        s_pTextures[StatusComponent::StatusEffectType::POISONED] = wolf::TextureManager::CreateTexture("data/textures/SEPoisoned.png");
        s_pTextures[StatusComponent::StatusEffectType::POISONED]->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
    }
    s_iComponentCounter++;

    for(int i = 0; i < StatusEffectType::NONE; i++)
    {
        this->m_aStatusEffects[i].m_OwnerComponent = this;
        this->m_aStatusEffects[i].m_StatusEffectType = (StatusEffectType)i;
    }

    wolf::EventManager::AddListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

StatusComponent::~StatusComponent()
{
    wolf::EventManager::RemoveListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
    s_iComponentCounter--;
    if(s_iComponentCounter == 0)
    {
        
    }
}

// If status effect already present, reset timer
// else, add status effect
void StatusComponent::AddStatusEffect(StatusEffectType p_se_type, float p_lifespan)
{
    this->m_aStatusEffects[p_se_type].m_StatusEffectType = p_se_type;
    this->m_aStatusEffects[p_se_type].m_isActive = true;
    this->m_aStatusEffects[p_se_type].m_timer.Restart();
    this->m_aStatusEffects[p_se_type].m_fLifespan = p_lifespan;
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
            statusEffect.ApplyStatusEffect(p_delta);

            if(statusEffect.m_fLifespan >= 0 && statusEffect.m_timer.Elapsed() >= statusEffect.m_fLifespan)
            {
                std::cout << "StatusComponent - Delete status effect: " << statusEffect.m_StatusEffectType << std::endl;
                
                this->RemoveStatusEffect(statusEffect.m_StatusEffectType);
            }
        }
    }
}

void StatusComponent::RemoveStatusEffect(StatusEffectType p_se_type)
{
    this->m_aStatusEffects[p_se_type].m_isActive = false;

    if(p_se_type == StatusEffectType::PETRIFIED)
    {
        AnimatedSprite2D* animatedSprite2DComponent = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
        if(animatedSprite2DComponent != nullptr)
        {
            animatedSprite2DComponent->SetTint(glm::vec3(1.0f));
        }
    }
}

void StatusComponent::RenderPlayerSEIcons()
{
    if(this->GetGameObject()->HasAny<PlayerController>())
    {
        int activeSECount = 0;
        if(this->IsStatusEffectActive(StatusEffectType::BURNING)) activeSECount++;
        if(this->IsStatusEffectActive(StatusEffectType::PETRIFIED)) activeSECount++;
        if(this->IsStatusEffectActive(StatusEffectType::POISONED)) activeSECount++;

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoBackground;
        ImVec2 windowSize = activeSECount == 0 ? ImVec2(0.0f, 0.0f) : ImVec2((s_vTextureSize.x + 16) * activeSECount + 8, s_vTextureSize.y + 24);
        ImGui::SetNextWindowPos({10, 10});
        ImGui::SetNextWindowSize(windowSize);
        ImGui::Begin("\t", nullptr, flags);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

        if(m_aStatusEffects[StatusEffectType::BURNING].m_isActive)
        {
            float lifetime = m_aStatusEffects[StatusEffectType::BURNING].m_timer.Elapsed();
            float lifespan = m_aStatusEffects[StatusEffectType::BURNING].m_fLifespan;
            if (ImGui::ImageButton("SE", (void*)(intptr_t)s_pTextures[StatusEffectType::BURNING]->GetID(), s_vTextureSize)) {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) 
            {
                // We display the details string that we constructed earlier
                ImGui::BeginTooltip();
                if(lifespan > lifetime) ImGui::Text("%s\n%f", "You Are Burning" , lifespan - lifetime);
                else ImGui::Text("%s\n%s","You Are Burning","inf");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
        }

        if(m_aStatusEffects[StatusEffectType::PETRIFIED].m_isActive)
        {
            float lifetime = m_aStatusEffects[StatusEffectType::PETRIFIED].m_timer.Elapsed();
            float lifespan = m_aStatusEffects[StatusEffectType::PETRIFIED].m_fLifespan;
            if (ImGui::ImageButton("SE", (void*)(intptr_t)s_pTextures[StatusEffectType::PETRIFIED]->GetID(), s_vTextureSize)) {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) 
            {
                // We display the details string that we constructed earlier
                ImGui::BeginTooltip();
                if(lifespan > lifetime) ImGui::Text("%s\n%f", "You Are Petrified" , lifespan - lifetime);
                else ImGui::Text("%s\n%s","You Are Petrified","inf");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
        } 

        if(m_aStatusEffects[StatusEffectType::POISONED].m_isActive)
        {
            float lifetime = m_aStatusEffects[StatusEffectType::POISONED].m_timer.Elapsed();
            float lifespan = m_aStatusEffects[StatusEffectType::POISONED].m_fLifespan;
            if (ImGui::ImageButton("SE", (void*)(intptr_t)s_pTextures[StatusEffectType::POISONED]->GetID(), s_vTextureSize)) {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) 
            {
                // We display the details string that we constructed earlier
                ImGui::BeginTooltip();
                if(lifespan > lifetime) ImGui::Text("%s\n%f", "You Are Poisoned" , lifespan - lifetime);
                else ImGui::Text("%s\n%s","You Are Poisoned","inf");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
        }
        ImGui::PopStyleColor(3);
        
        
        ImGui::End();
    }
}

void StatusComponent::StatusEffect::ApplyStatusEffect(float p_delta)
{
    switch (this->m_StatusEffectType)
    {
        case StatusEffectType::BURNING:
        {
            HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
            if(health != nullptr)
            {
                health->Pierce(100.0f * p_delta);
            }
            else
            {
                std::cout << "StatusComponent - ERROR: HealthComponent not found." << std::endl;
            }
            break;
        }

        case StatusEffectType::PETRIFIED:
        {
            VelocityComponent* velocityComponent = this->m_OwnerComponent->GetGameObject()->GetComponent<VelocityComponent>();
            if(velocityComponent != nullptr)
            {
                velocityComponent->SetVelocity(glm::vec2(0.0f, 0.0f));
            }

            AnimatedSprite2D* animatedSprite2DComponent = this->m_OwnerComponent->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if(animatedSprite2DComponent != nullptr)
            {
                animatedSprite2DComponent->SetTint(glm::vec3(1.5f, 1.5f, 1.5f));
            }

            break;
        }      
        
        case StatusEffectType::POISONED:
        {
            HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
            if(health != nullptr)
            {
                health->Pierce(50.0f * p_delta);
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
    this->AddStatusEffect(static_cast<StatusComponent::StatusEffectType>(p_event.iType), p_event.fDuration);
}