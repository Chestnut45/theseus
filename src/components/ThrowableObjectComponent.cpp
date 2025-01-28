#include "ThrowableObjectComponent.h"
#include "PlayerController.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include <imgui/imgui.h>
#include <iostream>

ThrowableObjectComponent::ThrowableObjectComponent(float damage, ColliderManager* colliderManager)
    : m_damage(damage), m_state(ThrowableState::IDLE), m_pColliderManager(colliderManager) 
{
}

void ThrowableObjectComponent::Update(float delta) {
    m_uiPlayerGOId = GetGameObject()->GetScene().GetPlayerID();

    if (!m_pCollider) m_pCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!m_pTransform) m_pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();

    if(m_pCollider != nullptr)
    {
        m_pCollider->SetIgnoreTag(m_uiPlayerGOId);
    }

    switch (m_state) {
        case ThrowableState::IDLE:
            m_pCollider->SetActive(false);
            if (IsCloseToPlayer(150.0f)) {
                m_hoverAnimationOffset = sin(ImGui::GetTime() * 3.0f) * 5.0f;
                RenderPickupPrompt();
            }
            break;

        case ThrowableState::PICKED_UP:
            m_pCollider->SetActive(false);
            FollowPlayer();
            break;

        case ThrowableState::THROWN:
            m_pCollider->SetActive(true);
            HandleCollision();
            CheckLifetime(delta);
            break;
    }
}
void ThrowableObjectComponent::PickUp() {
    if (m_state == ThrowableState::IDLE) {
        m_state = ThrowableState::PICKED_UP;
        // std::cout << "Throwable object picked up!" << std::endl;
    }
}

void ThrowableObjectComponent::Drop() {
    if (m_state == ThrowableState::PICKED_UP) {
        m_state = ThrowableState::IDLE;
        // std::cout << "Throwable object dropped!" << std::endl;
    }
}

void ThrowableObjectComponent::SetThrown() {
    if (m_state == ThrowableState::PICKED_UP) {
        m_state = ThrowableState::THROWN;
        // std::cout << "Throwable object thrown!" << std::endl;
    }
}

bool ThrowableObjectComponent::IsCloseToPlayer(float distanceThreshold) const {
    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerTransform = playerController.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (playerTransform && m_pTransform) {
            float distance = glm::distance(playerTransform->GetGlobalPosition(), m_pTransform->GetGlobalPosition());
            return distance < distanceThreshold;
        }
    }
    return false;
}

void ThrowableObjectComponent::RenderPickupPrompt() {
    // Set screen-space position for the pickup prompt
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 promptPosition = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.8f);  // Centered horizontally, lower portion vertically

    ImGui::SetNextWindowPos(promptPosition, ImGuiCond_Always, ImVec2(0.5f, 0.5f));  // Centered alignment
    ImGui::SetNextWindowBgAlpha(0.85f);

    // Pulse color and size animation for visual feedback
    float alphaPulse = 0.6f + 0.4f * sin(ImGui::GetTime() * 3.0f);
    ImVec4 glowColor = ImVec4(0.8f, 0.92f, 0.3f, alphaPulse); // Neon green glow

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Text, glowColor);
    ImGui::PushStyleColor(ImGuiCol_Border, glowColor);

    ImGui::Begin("PickUpPrompt", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Press E to pick up");
    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}
void ThrowableObjectComponent::FollowPlayer() {
    if (!m_pTransform) return;

    // Retrieve the player’s direction
    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerTransform = playerController.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (!playerTransform) continue;

        glm::vec2 offset;

        // Adjust offset based on player's facing direction
        switch (playerController.GetLastFacingDirection()) { 
            case PlayerController::PlayerDirection::EAST:        offset = glm::vec2(45.0f, 0.0f); break;
            case PlayerController::PlayerDirection::WEST:        offset = glm::vec2(-45.0f, 0.0f); break;
            case PlayerController::PlayerDirection::NORTH:       offset = glm::vec2(0.0f, 60.0f); break;
            case PlayerController::PlayerDirection::SOUTH:       offset = glm::vec2(0.0f, -60.0f); break;
            case PlayerController::PlayerDirection::NORTH_EAST:  offset = glm::vec2(35.0f, 35.0f); break; // Reduced by 10 units
            case PlayerController::PlayerDirection::NORTH_WEST:  offset = glm::vec2(-35.0f, 35.0f); break; // Reduced by 10 units
            case PlayerController::PlayerDirection::SOUTH_EAST:  offset = glm::vec2(35.0f, -35.0f); break; // Reduced by 10 units
            case PlayerController::PlayerDirection::SOUTH_WEST:  offset = glm::vec2(-35.0f, -35.0f); break; // Reduced by 10 units
            default:                                             offset = glm::vec2(45.0f, 0.0f); break; // Default to right
        }

        // Smoothly update the object's position relative to the player with the calculated offset
        glm::vec2 targetPosition = playerTransform->GetGlobalPosition() + offset;
        m_pTransform->SetPosition(glm::mix(m_pTransform->GetGlobalPosition(), targetPosition, 0.1f)); // 0.1f for smooth following
    }
}

void ThrowableObjectComponent::SetState(ThrowableState newState) {
    m_state = newState;
}

void ThrowableObjectComponent::HandleCollision() {
    if (m_hasCollided) return; // Prevent double collision handling

    // Check collision with Minotaurs
    for (auto&& [_, minitaurController] : GetGameObject()->GetScene().Each<MinitaurController>()) {
        auto* minitaurObject = minitaurController.GetGameObject();
        auto* minitaurCollider = minitaurObject->GetComponent<ColliderComponent>();

        if (minitaurCollider && m_pCollider && m_pColliderManager->IsColliding(*m_pCollider, *minitaurCollider, 0.0f)) {
            HandleEnemyCollision(minitaurObject, 200.0f); // Apply damage to Minotaur
            return;
        }
    }

    // Check collision with Gorgons
    for (auto&& [_, gorgonController] : GetGameObject()->GetScene().Each<GorgonController>()) {
        auto* gorgonObject = gorgonController.GetGameObject();
        auto* gorgonCollider = gorgonObject->GetComponent<ColliderComponent>();

        if (gorgonCollider && m_pCollider && m_pColliderManager->IsColliding(*m_pCollider, *gorgonCollider, 0.0f)) {
            HandleEnemyCollision(gorgonObject, 150.0f); // Apply damage to Gorgon
            return;
        }
    }

    // Check collision with Harpies
    for (auto&& [_, harpyController] : GetGameObject()->GetScene().Each<HarpyController>()) {
        auto* harpyObject = harpyController.GetGameObject();
        auto* harpyCollider = harpyObject->GetComponent<ColliderComponent>();

        if (harpyCollider && m_pCollider && m_pColliderManager->IsColliding(*m_pCollider, *harpyCollider, 0.0f)) {
            HandleEnemyCollision(harpyObject, 100.0f); // Apply damage to Harpy
            return;
        }
    }
}

void ThrowableObjectComponent::HandleEnemyCollision(wolf::GameObject* enemyObject, float damage) {
    auto* healthComponent = enemyObject->GetComponent<HealthComponent>();
    if (healthComponent) {
        healthComponent->Damage(damage); // Apply specified damage
        // std::cout << "Collision with enemy! Damage dealt: " << damage << std::endl;
    }
    m_hasCollided = true;
    GetGameObject()->Delete(); // Mark the throwable object for deletion
}

void ThrowableObjectComponent::CheckLifetime(float delta) {
    if (m_hasCollided) return; // Skip if collision has already occurred

    m_lifetime -= delta;
    if (m_lifetime <= 0.0f) {
        GetGameObject()->Delete(); // Destroy object after timeout
        // std::cout << "Throwable object destroyed due to timeout" << std::endl;
    }
}
