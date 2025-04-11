//-----------------------------------------------------------------------------
// File:			TrapComponent.cpp
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for spike traps
//-----------------------------------------------------------------------------
#include "TrapComponent.h"
#include "PlayerController.h"
#include "MinitaurController.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include "HealthComponent.h"

TrapComponent::TrapComponent(TriggerComponent* pCreatorTrigger, float damage, float lifespan, ColliderManager* pColliderManager, float initialDelay)
    : m_damage(damage), m_lifespan(lifespan), m_pColliderManager(pColliderManager), m_pCreatorTrigger(pCreatorTrigger), m_initialDelay(initialDelay) {
    // Start timers immediately, making the trap active upon creation
    m_lifespanTimer.Start();
    m_entityTypes = m_pCreatorTrigger->GetEntityListenTypes();
}

void TrapComponent::Update(float delta) {
    if (!m_isActive) return;

    // Check if the lifespan has expired
    if (m_lifespanTimer.Elapsed() >= m_lifespan) {
        // Send event before deleting the trap
        // wolf::Log("Triggering TriggerPurposeFinishedEvent for GameObject " + std::to_string(GetGameObject()->GetID()));
        wolf::EventManager::TriggerEvent(TriggerPurposeFinishedEvent(m_pCreatorTrigger));
        GetGameObject()->Delete();
        return;
    }

    // Handle collision
    if (CheckForPlayerCollision(delta) || CheckForEnemyCollision(delta)) {
        m_triggered = true;
    }
}

bool TrapComponent::CheckForPlayerCollision(float delta)
{
    if (!(m_entityTypes & EntityListenType::PLAYER || m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING)) return false;

    auto* trapCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!trapCollider) return false;

    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        // Ignore player if rolling
        if (m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING &&
            playerController.GetPlayerAction() == PlayerController::PlayerAction::ROLLING)
        {
            continue;
        }

        // Get player object
        auto* pObj = playerController.GetGameObject();
        auto* playerCollider = pObj->GetComponent<ColliderComponent>();
        if (playerCollider &&
            playerCollider->IsActive() &&
            playerCollider->IsHurtbox() &&
            m_pColliderManager->IsColliding(*trapCollider, *playerCollider, delta))
        {
            auto* playerHealth = pObj->GetComponent<HealthComponent>();
            if (playerHealth) {

                // Damage the player
                playerHealth->Damage(m_damage);

                // Apply knockback to the player
                auto* playerVelocity = pObj->GetComponent<VelocityComponent>();
                if (playerVelocity)
                {
                    // Calculate knockback direction
                    const glm::vec2 targetPosition = pObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    const glm::vec2 currentPosition = GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                    playerVelocity->ApplyKnockback(knockbackDirection, 2000);
                }

                return true;
            }
        }
    }
    return false;
}

bool TrapComponent::CheckForEnemyCollision(float delta) 
{
    auto* trapCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!trapCollider) return false;

    // Get position
    const glm::vec2 currentPosition = GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Minitaurs
    if (m_entityTypes & EntityListenType::MINITAUR)
    {
        for (auto&& [_, minitaur] : GetGameObject()->GetScene().Each<MinitaurController>())
        {
            // Get object
            auto* pObj = minitaur.GetGameObject();
            auto* pCollider = pObj->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_pColliderManager->IsColliding(*trapCollider, *pCollider, delta))
            {
                auto* pHealth = pObj->GetComponent<HealthComponent>();
                if (pHealth)
                {
                    // Apply damage
                    pHealth->Damage(m_damage);

                    // Apply knockback
                    auto* pVelocity = pObj->GetComponent<VelocityComponent>();
                    if (pVelocity)
                    {
                        // Calculate knockback direction
                        const glm::vec2 targetPosition = pObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                        glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                        pVelocity->ApplyKnockback(knockbackDirection, 2000);
                    }

                    return true;
                }
            }
        }
    }

    // Gorgons
    if (m_entityTypes & EntityListenType::GORGON)
    {
        for (auto&& [_, gorgon] : GetGameObject()->GetScene().Each<GorgonController>())
        {
            // Get object
            auto* pObj = gorgon.GetGameObject();
            auto* pCollider = pObj->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_pColliderManager->IsColliding(*trapCollider, *pCollider, delta))
            {
                auto* pHealth = pObj->GetComponent<HealthComponent>();
                if (pHealth)
                {
                    // Apply damage
                    pHealth->Damage(m_damage);

                    // Apply knockback
                    auto* pVelocity = pObj->GetComponent<VelocityComponent>();
                    if (pVelocity)
                    {
                        // Calculate knockback direction
                        const glm::vec2 targetPosition = pObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                        glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                        pVelocity->ApplyKnockback(knockbackDirection, 2000);
                    }

                    return true;
                }
            }
        }
    }

    // Harpies
    if (m_entityTypes & EntityListenType::HARPY)
    {
        for (auto&& [_, harpy] : GetGameObject()->GetScene().Each<HarpyController>())
        {
            // Get object
            auto* pObj = harpy.GetGameObject();
            auto* pCollider = pObj->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_pColliderManager->IsColliding(*trapCollider, *pCollider, delta))
            {
                auto* pHealth = pObj->GetComponent<HealthComponent>();
                if (pHealth)
                {
                    // Apply damage
                    pHealth->Damage(m_damage);

                    // Apply knockback
                    auto* pVelocity = pObj->GetComponent<VelocityComponent>();
                    if (pVelocity)
                    {
                        // Calculate knockback direction
                        const glm::vec2 targetPosition = pObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                        glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                        pVelocity->ApplyKnockback(knockbackDirection, 2000);
                    }

                    return true;
                }
            }
        }
    }
    return false;
}