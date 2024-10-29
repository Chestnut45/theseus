#include "ThrowableObjectComponent.h"
#include "PlayerController.h"
#include <imgui/imgui.h>
#include <iostream>

ThrowableObjectComponent::ThrowableObjectComponent(float damage, ColliderManager* colliderManager)
    : m_damage(damage), m_state(ThrowableState::IDLE), m_pColliderManager(colliderManager) {}

void ThrowableObjectComponent::Update(float delta) {
    if (!m_pCollider) m_pCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!m_pTransform) m_pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();

    switch (m_state) {
        case ThrowableState::IDLE:
            if (IsCloseToPlayer(150.0f)) {
                m_hoverAnimationOffset = sin(ImGui::GetTime() * 3.0f) * 5.0f;
                RenderPickupPrompt();
            }
            break;

        case ThrowableState::PICKED_UP:
            FollowPlayer();
            break;

        case ThrowableState::THROWN:
            // In THROWN state, no specific behavior here; PlayerController handles movement
            break;
    }
}
void ThrowableObjectComponent::PickUp() {
    if (m_state == ThrowableState::IDLE) {
        m_state = ThrowableState::PICKED_UP;
        std::cout << "Throwable object picked up!" << std::endl;
    }
}

void ThrowableObjectComponent::Drop() {
    if (m_state == ThrowableState::PICKED_UP) {
        m_state = ThrowableState::IDLE;
        std::cout << "Throwable object dropped!" << std::endl;
    }
}

void ThrowableObjectComponent::SetThrown() {
    if (m_state == ThrowableState::PICKED_UP) {
        m_state = ThrowableState::THROWN;
        std::cout << "Throwable object thrown!" << std::endl;
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
    if (m_pTransform) {
        auto position = m_pTransform->GetGlobalPosition();
        ImVec2 promptPosition = ImVec2(position.x, position.y - 40.0f + m_hoverAnimationOffset);

        ImGui::SetNextWindowPos(promptPosition);
        ImGui::SetNextWindowBgAlpha(0.85f);

        // Pulse color and size animation
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
}

void ThrowableObjectComponent::FollowPlayer() {
    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerTransform = playerController.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (playerTransform && m_pTransform) {
            m_pTransform->SetPosition(playerTransform->GetGlobalPosition() + glm::vec2(10.0f, 0.0f)); // Offset slightly to the right
        }
    }
}

void ThrowableObjectComponent::SetState(ThrowableState newState) {
    m_state = newState;
}
