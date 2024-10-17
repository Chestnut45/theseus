//-----------------------------------------------------------------------------
// File: WeaponComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Base for weapons.
//-----------------------------------------------------------------------------

#include "WeaponComponent.h"

#include "ColliderComponent.h"
#include "PlayerController.h"
#include "VelocityComponent.h"

WeaponComponent::WeaponComponent(WeaponType p_weapon_type)
{
    this->m_WeaponType = p_weapon_type;
}

void WeaponComponent::Attack()
{
    switch(this->m_WeaponType)
    {
        case WeaponType::CROSSBOW:
        auto& scene = this->GetGameObject()->GetScene();
        auto& projectile = scene.CreateObject2D();
        auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 0, 1);
        auto& projectileVelocity = projectile.AddComponent<VelocityComponent>();

        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition()));

        projectileCollider.SetDamage(10.0f);
        projectileCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));

        PlayerController* playerController = this->GetGameObject()->GetComponent<PlayerController>();
        if(playerController != nullptr)
        {
            //projectileVelocity.SetVelocity();
        }
        
    }
}