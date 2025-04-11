//-----------------------------------------------------------------------------
// File:			BoulderTrapComponent.cpp
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for the boulder traps
//-----------------------------------------------------------------------------
#include "BoulderTrapComponent.h"
#include "W_GameObject.h"
#include "W_EventManager.h"
#include "W_Transform2D.h"
#include "VelocityComponent.h"
#include "W_Sprite2D.h"
#include "PlayerController.h"
#include "TriggerPurposeFinishedEvent.h"
#include "GorgonController.h"
#include "MinitaurController.h"
#include <unordered_set>

// Constants for effects
constexpr float FADE_TRIGGER_THRESHOLD = 1.0f; // Time before lifespan ends to trigger cool effect
constexpr float SCALE_REDUCTION_SPEED = 0.5f;  // Speed of scale reduction (per second)
constexpr float GLOW_PULSE_SPEED = 5.0f;       // Speed of glow pulsing (oscillation rate)
constexpr float FADE_OUT_SPEED = 2.0f;         // Speed of fade-out (opacity reduction)
constexpr float MIN_SCALE = 0.1f;              // Minimum allowable scale to prevent shrinking to zero

BoulderTrapComponent::BoulderTrapComponent(TriggerComponent* trigger, ColliderManager* colliderManager, BoulderDirection direction, float speed, float lifespan)
    : m_pTrigger(trigger),
      m_colliderManager(colliderManager),
      m_direction(direction),
      m_speed(speed),
      m_lifespan(lifespan),
      m_timerStarted(false),
      m_fadingEffectTriggered(false) {}

void BoulderTrapComponent::Update(float delta) {
    // Initialize timer and velocity if not started
    if (!m_timerStarted) {
        InitializeBoulder();
    }

    float elapsed = m_lifespanTimer.Elapsed();

     // **Check for wall collision and delete if colliding**
    if (CheckForWallCollision(delta)) {
        wolf::EventManager::TriggerEvent(TriggerPurposeFinishedEvent(m_pTrigger));
        GetGameObject()->Delete();
        return; 
    }

    // Check for Entity collision
    if (CheckForEntityCollision(delta)) {
    }

    // Trigger cool effects if lifespan is near expiry
    if (!m_fadingEffectTriggered && elapsed >= m_lifespan - FADE_TRIGGER_THRESHOLD) {
        m_fadingEffectTriggered = true;
    }

    // Apply cool effects (e.g., scaling, glowing, fading)
    if (m_fadingEffectTriggered) {
        ApplyCoolEffect(delta, elapsed);
    }

    // Delete boulder if lifespan is expired
    if (elapsed >= m_lifespan) {
        wolf::EventManager::TriggerEvent(TriggerPurposeFinishedEvent(m_pTrigger));
        GetGameObject()->Delete();
    }
}

void BoulderTrapComponent::InitializeBoulder() {
    
    if (!m_pLabyrinthManager) {
        for (auto&& [_, lm] : GetGameObject()->GetScene().Each<LabyrinthManager>()) {
            m_pLabyrinthManager = &lm;
            break;
        }
    }

    m_lifespanTimer.Start();
    m_timerStarted = true;

    // Set initial velocity
    auto* velocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (velocity) {
        glm::vec2 initialVelocity = CalculateInitialVelocity();
        velocity->SetVelocity(initialVelocity);
        // wolf::Log("BoulderTrapComponent: Initial velocity set.");
    } else {
        // wolf::Log("BoulderTrapComponent: Missing VelocityComponent.");
    }
}

glm::vec2 BoulderTrapComponent::CalculateInitialVelocity() const {
    switch (m_direction) {
        case BoulderDirection::UP:    return { 0.0f, m_speed };  // Positive Y moves down
        case BoulderDirection::DOWN:  return { 0.0f, -m_speed }; // Negative Y moves up
        case BoulderDirection::LEFT:  return { -m_speed, 0.0f }; // Moving left decreases X
        case BoulderDirection::RIGHT: return { m_speed, 0.0f };  // Moving right increases X
        default:                      return { 0.0f, 0.0f };     // Safe fallback
    }
}


void BoulderTrapComponent::ApplyCoolEffect(float delta, float elapsed) {
    auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
    auto* sprite = GetGameObject()->GetComponent<wolf::Sprite2D>();

    if (!transform) {
        // wolf::Log("BoulderTrapComponent: Missing Transform2D for CoolEffect.");
        return;
    }

    // Apply scale reduction
    glm::vec2 currentScale = transform->GetGlobalScale();
    if (currentScale.x > MIN_SCALE && currentScale.y > MIN_SCALE) {
        glm::vec2 newScale = currentScale - (glm::vec2(SCALE_REDUCTION_SPEED) * delta);
        newScale = glm::max(newScale, glm::vec2(MIN_SCALE)); // Clamp to MIN_SCALE
        transform->SetScale(newScale);
        // wolf::Log("BoulderTrapComponent: Scale reduced.");
    }

    if (!sprite) {
        // wolf::Log("BoulderTrapComponent: Missing Sprite2D for CoolEffect.");
        return;
    }

    // Apply glow effect (dynamic pulsing tint color)
    float glowIntensity = 0.5f + 0.5f * sin(elapsed * GLOW_PULSE_SPEED); // Oscillate between 0.5 and 1.0
    glm::vec3 baseTint(1.0f, 1.0f - glowIntensity * 0.3f, 1.0f - glowIntensity * 0.6f); // Tint changes from white to a subtle red
    sprite->SetTint(baseTint);

    // Apply fade-out transparency (dynamic alpha blending)
    float fadeProgress = (elapsed - (m_lifespan - FADE_TRIGGER_THRESHOLD)) / FADE_TRIGGER_THRESHOLD;
    if (fadeProgress > 0.0f) {
        float newAlpha = glm::max(1.0f - (fadeProgress * FADE_OUT_SPEED), 0.0f);
        glm::vec3 fadedTint = baseTint * glm::vec3(newAlpha); // Multiply tint with alpha for overall fading
        sprite->SetTint(fadedTint);
        // wolf::Log("BoulderTrapComponent: Fade-out effect applied with alpha: " + std::to_string(newAlpha));
    }

    // wolf::Log("BoulderTrapComponent: Glow effect applied with intensity: " + std::to_string(glowIntensity));
}

bool BoulderTrapComponent::CheckForEntityCollision(float delta) {
    auto* boulderCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!boulderCollider) return false;

    bool collisionDetected = false;

    // Check collision for all damageable entities
    collisionDetected |= CheckAndHandleCollision<PlayerController>(delta, boulderCollider);
    collisionDetected |= CheckAndHandleCollision<MinitaurController>(delta, boulderCollider);
    collisionDetected |= CheckAndHandleCollision<GorgonController>(delta, boulderCollider);

    return collisionDetected;
}

template <typename T>
bool BoulderTrapComponent::CheckAndHandleCollision(float delta, ColliderComponent* boulderCollider) {
    bool detected = false;

    for (auto&& [_, entityController] : GetGameObject()->GetScene().Each<T>()) {
        auto* entityObj = entityController.GetGameObject();
        auto* entityCollider = entityObj->template GetComponent<ColliderComponent>(); // FIXED

        if (entityCollider &&
            entityCollider->IsActive() &&
            entityCollider->IsHurtbox() &&
            m_colliderManager->IsColliding(*boulderCollider, *entityCollider, delta)) {

            auto* entityHealth = entityObj->template GetComponent<HealthComponent>(); // FIXED
            if (entityHealth) {
                entityHealth->Damage(m_damage);
                detected = true;

                // Apply knockback
                auto* entityVelocity = entityObj->template GetComponent<VelocityComponent>(); // FIXED
                if (entityVelocity) {
                    glm::vec2 entityPosition = entityObj->template GetComponent<wolf::Transform2D>()->GetGlobalPosition(); // FIXED
                    glm::vec2 boulderPosition = GetGameObject()->template GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    glm::vec2 knockbackDirection = glm::normalize(entityPosition - boulderPosition);
                    entityVelocity->ApplyKnockback(knockbackDirection, m_knockbackForce);
                }
            }
        }
    }

    return detected;
}

bool BoulderTrapComponent::CheckForWallCollision(float delta) {
    if (!m_pLabyrinthManager || !m_colliderManager) return false; // Ensure managers exist

    auto* boulderCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!boulderCollider) return false;

    // **Get the boulder's world position and apply an offset**
    auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
    glm::vec2 worldPosition = transform ? transform->GetGlobalPosition() : glm::vec2(0);
    
    constexpr float offset = 35.0f; // **Apply a slight positional shift**
    worldPosition += glm::vec2(offset, offset);

    // **Convert world position to tile position**
    glm::ivec2 tilePosition = m_pLabyrinthManager->GetTilePosition(worldPosition);
    int tileID = m_pLabyrinthManager->GetTile(tilePosition.x, tilePosition.y);

    // **Define all possible wall tile IDs**
    static const std::unordered_set<int> wallTiles = {
        Tile::WallBottomLeft, Tile::WallBottomRight, Tile::WallBottom, Tile::WallChest,
        Tile::WallHelmet, Tile::WallLeft, Tile::WallMaze, Tile::WallMinotaur, Tile::WallPillars,
        Tile::WallPot, Tile::WallRight, Tile::WallSpiral, Tile::WallSquare,
        Tile::WallTopLeft, Tile::WallTopRight, Tile::WallTop
    };

    // **Check if the tile is a wall**
    if (wallTiles.find(tileID) != wallTiles.end()) {
        // **Ensure the boulder is actually colliding with the wall collider**
        for (auto&& [_, wallCollider] : GetGameObject()->GetScene().Each<ColliderComponent>()) {
            if (wallCollider.IsActive() &&
                m_colliderManager->IsColliding(*boulderCollider, wallCollider, delta)) {
                return true;
            }
        }
    }

    return false; // No collision detected
}



