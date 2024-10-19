//-----------------------------------------------------------------------------
// File: WeaponComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Base for weapons.
//-----------------------------------------------------------------------------

#include "WeaponComponent.h"

#include "ColliderComponent.h"
#include "PlayerController.h"
#include "VelocityComponent.h"

int WeaponComponent::s_iComponentCount = 0;
float WeaponComponent::s_aAttackDelays[WeaponComponent::WeaponType::NONE];

WeaponComponent::WeaponComponent()
{
    this->m_CurrentWeapon = WeaponComponent::WeaponType::NONE;

    for(int i = 0; i < WeaponComponent::WeaponType::NONE; i++)
    {
        this->m_aAvailableWeapons[i] = 0;
    }

    this->m_aAvailableWeapons[WeaponComponent::WeaponType::NONE] = 1;

    if(s_iComponentCount == 0)
    {
        s_aAttackDelays[WeaponComponent::WeaponType::BOW] = 1.0f;
        s_aAttackDelays[WeaponComponent::WeaponType::CROSSBOW] = 0.5f;
        s_aAttackDelays[WeaponComponent::WeaponType::SWORD] = 0.5f;
        s_aAttackDelays[WeaponComponent::WeaponType::NONE] = 0.5f;
    }
    s_iComponentCount++; 
}

void WeaponComponent::Attack()
{
    glm::vec2 playerVelocity = glm::vec2(0.0f);
        
    switch(this->m_CurrentWeapon)
    {
        case WeaponType::CROSSBOW:
        {
            auto& scene = this->GetGameObject()->GetScene();
            auto& projectile = scene.CreateObject2D();
            auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
            auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
            auto& projectileVelocity = projectile.AddComponent<VelocityComponent>();

            projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
            projectile.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition()));

            projectileCollider.SetDamage(10.0f);
            projectileCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));

            PlayerController* playerController = this->GetGameObject()->GetComponent<PlayerController>();
            VelocityComponent* playerVelocityComponent = this->GetGameObject()->GetComponent<VelocityComponent>();

            if(playerController != nullptr)
            {
                if(playerVelocityComponent != nullptr)
                {
                    playerVelocity = playerVelocityComponent->GetVelocity();
                }
                else
                {
                    playerVelocity = glm::vec2(0.0f, 0.0f);
                }
                projectileVelocity.SetVelocity(playerController->GetDirectionVector() * 256.0f + playerVelocity);
            }
            break;
        }
    }
}

void WeaponComponent::CollectWeapon(WeaponComponent::WeaponType p_weapon_type)
{
    this->m_CurrentWeapon = p_weapon_type;
    this->m_aAvailableWeapons[p_weapon_type] = 1;
}

void WeaponComponent::SwitchToNextWeapon()
{
    int i = this->m_CurrentWeapon;
    while (true)
    {
        i = (i + 1) % (WeaponComponent::WeaponType::NONE + 1);

        if(this->m_aAvailableWeapons[i] == 1)
        {
            this->m_CurrentWeapon = static_cast<WeaponType>(i);
            break;
        }
        if(i == this->m_CurrentWeapon)
        {
            break;
        }
        
    }
}

float WeaponComponent::GetWeaponAttackDelay(WeaponType p_weapon_type)
{
    return s_aAttackDelays[p_weapon_type];
}