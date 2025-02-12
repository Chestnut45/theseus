#include "BossController.h"
#include <W_GameObject.h>
#include <W_Transform2D.h>

#include <VelocityComponent.h>
#include <AnimatedSprite2D.h>
#include <HealthComponent.h>
#include <StatusComponent.h>
#include <ColliderComponent.h>
#include <PlayerController.h>
#include <HomingComponent.h>
#include <LabyrinthManager.h>
#include <HomingComponent.h>
#include <W_Timer.h>
#include <W_EventManager.h>

#include "../ColliderManager.h"

#include "GLShapesRenderer.h"
#include "../TileFireManager.h"

#include <DDACalculator.h>

#include <wolf.h>

#include <AttackDamageComponent.h>
#include <TimedDestroyerComponent.h>
#include <GorgonBuilder.h>
#include <MinitaurBuilder.h>
#include <HarpyBuilder.h>
#include "../LabyrinthTiles.h"

BossController::BossController()
{
    wolf::EventManager::AddListener<DamageEvent, BossController, &BossController::OnDamageEvent>(*this);
}

BossController::~BossController()
{
    wolf::EventManager::RemoveListener<DamageEvent, BossController, &BossController::OnDamageEvent>(*this);
}

void BossController::Init()
{
    // Initialize stats
    m_active = false;
    m_maxHealth = 6000;
    m_prevHealthFraction = 1.0f;

    // Phase 1 stats
    m_throneBlockRange = 300;
    m_numSummons = 10;
    m_forcefieldRadius = 500.0f;
    m_slowdownFactor = 0.5f;
    m_currentWave = 0;
    m_remainingEnemies = 0;
    m_waveTransitionTimer = 0.0f;
    m_waveActive = false;

    // Phase 2 stats
    m_attackChain = 0;
    m_prevAttack = -1;
    m_axeAttackDamage = 100;
    m_slamAttackDamage = 125;
    m_minDistToPlayer = 160;
    m_maxDistToPlayer = 280;
    m_strafeClockwise = true;
    m_axeSummoned = false;
    m_slamStun = false;
    m_strafeSpeed = 160.0f;
    m_chaseSpeed = 240.0f;
    m_shadowDistance = 26.0f;
    m_altitude = 0.0f;

    // Phase 3 stats
    m_fireBreathWindupTime = 0.75f; // Seconds
    m_fireBreathWindupTimer = 1.0f; // Seconds
    m_chargeWindupTint = glm::vec3(3.0f, 1.0f, 1.0f);
    m_fireBreathDamage = 10; // Per projectile
    m_fireBreathRange = 300;
    m_fireBreathDuration = 10.0f;
    m_fireBreathTurningCapRadian = 7.5f * (M_PI / 180.0f); // Maximum angle for each turn instance
    m_fireBreathTurningDelay = 0.2f;      // Delay between each turn instance
    m_fireBreathTurningTimer = 0.0f;
    m_lastDirection = glm::vec2(1.0f, 0.0f);

    m_chargeWindupTime = 1.0f; // Seconds
    m_chargeWindupTimer = 1.0f; // Seconds
    m_chargeWindupTint = glm::vec3(4.0f, 4.0f, 4.0f);
    m_chargeAttackDamage = 60;
    m_chargeAttackRange = 2000;
    m_stunTimer = 0.5f; // Seconds
    m_chargeTurningCapDegree = 9.0f; // Degrees
    m_chargeTurningDelay = 0.2f; // Seconds
    m_chargeSpeed = 480.0f;
    m_chargeKnockbackForce = 10000.0f;
    m_chargeChainCount = 0;

    m_pullTime = 1.5f;               // Pull state members
    m_pullTimer = 0.0f;
    m_pullForce = 300.0f;

    m_searchSpeed = 250.0f;
    m_searchTimer = 2.0f; // Seconds

    m_idleTimer = 2.0f;
    m_autoAttackRange = 200.0f;

    // Create components and cache pointers
    wolf::GameObject* pObject = GetGameObject();

    // Create transform
    m_pTransform = pObject->GetComponent<wolf::Transform2D>();

    // Create velocity
    pObject->DeleteComponent<VelocityComponent>();
    m_pVelocity = &pObject->AddComponent<VelocityComponent>();

    // Create sprite
    pObject->DeleteComponent<AnimatedSprite2D>();
    m_pAnimSprite = &pObject->AddComponent<AnimatedSprite2D>("data/boss_anim_init.yaml");
    m_pAnimSprite->SetLayer(8);

    // Create health
    pObject->DeleteComponent<HealthComponent>();
    m_pHealth = &pObject->AddComponent<HealthComponent>(m_maxHealth);

    // Create status
    pObject->DeleteComponent<StatusComponent>();
    m_pStatus = &pObject->AddComponent<StatusComponent>();

    // Create collider
    pObject->DeleteComponent<ColliderComponent>();
    m_pCollider = &pObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, false, false);
    m_pCollider->AddColliderBox(glm::vec2(111, 182), glm::vec2(-52, 96));

    // Grab a reference to the labyrinth manager
    for (auto&&[_, manager] : pObject->GetScene().Each<LabyrinthManager>())
    {
        m_pLabyrinthManager = &manager;
        break;
    }
    if (!m_pLabyrinthManager)
    {
        wolf::Error("BossController couldn't find any labyrinth manager!");
    }

    // Grab a reference to the room the boss is currently in
    auto roomOptional = m_pLabyrinthManager->GetRoom(m_pLabyrinthManager->GetTilePosition(m_pTransform->GetGlobalPosition()));
    if (!roomOptional.has_value())
    {
        wolf::Error("BossController Init was called outside of a valid room!");
    }

    // Store center of chamber
    m_centerOfChamber = m_pLabyrinthManager->GetWorldPosition(roomOptional->m_bounds.m_origin + roomOptional->m_bounds.m_size / 2);

    // Create the boss pillar group object
    m_pBossPillarGroup = &pObject->GetScene().CreateObject2D();

    // Generate tile locations for pillars
    const wolf::IRectangle& r = roomOptional->m_bounds;
    glm::ivec2 locations[] =
    {
        glm::ivec2(r.m_origin.x + r.m_size.x * 0.25f, r.m_origin.y + r.m_size.y * 0.25f),
        glm::ivec2(r.m_origin.x + r.m_size.x * 0.25f, r.m_origin.y + r.m_size.y * 0.75f),
        glm::ivec2(r.m_origin.x + r.m_size.x * 0.75f, r.m_origin.y + r.m_size.y * 0.75f),
        glm::ivec2(r.m_origin.x + r.m_size.x * 0.75f, r.m_origin.y + r.m_size.y * 0.25f)
    };

    for (const auto& tile : locations)
    {
        // Spawn a pillar as a child object of the pillar group object
        wolf::GameObject& pillar = pObject->GetScene().CreateObject2D();
        m_pBossPillarGroup->AddChild(pillar);
        pillar.AddComponent<wolf::Sprite2D>("data/textures/tile_wall_minotaur.png");
        
        // Set position and scale
        auto& transform = *pillar.GetComponent<wolf::Transform2D>();
        transform.SetPosition(m_pLabyrinthManager->GetWorldPosition(tile));
        transform.SetScale(glm::vec2(3.0f));

        // Create collider
        auto& collider = pillar.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, false, false);
        collider.AddColliderBox(glm::vec2(96.0f), glm::vec2(0.0f, 96.0f));
    }
    
    // Find player controller
    for (auto&&[_, controller] : pObject->GetScene().Each<PlayerController>())
    {
        m_pPlayerController = &controller;
        m_pPlayerObject = controller.GetGameObject();
        break;
    }
        // Create homing - Added by Nhật
    pObject->DeleteComponent<HomingComponent>();
    m_pHoming = &pObject->AddComponent<HomingComponent>(m_pPlayerObject, m_chargeTurningCapDegree, m_chargeTurningDelay, false);
    if (!m_pPlayerController)
    {
        wolf::Error("Boss controller init could not find player controller!");
    }

    EnterPhase2();
}

// <----------------- GENERAL UPDATE METHODS ----------------->

void BossController::Update(float delta)
{
    if (!m_active) return;

    // Update previous state
    m_prevState = m_state;

    // Call phase-specific update method
    switch (m_phase)
    {
        case FightPhase::PHASE_1:
            UpdatePhase1(delta);
            break;
        
        case FightPhase::PHASE_2:
            UpdatePhase2(delta);
            break;
        
        case FightPhase::PHASE_3:
            UpdatePhase3(delta);
            break;
    }

    UpdateAnimation();
    RenderHealthBar(delta);
}

void BossController::UpdateAnimation()
{   
    // Compass direction animation names
    static const char* s_dirNames[] =
    {
        "East",
        "North",
        "West",
        "South"
    };

    // Query player spatial info
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 dirToPlayer = glm::normalize(playerPos - m_pTransform->GetGlobalPosition());
    float angle = -glm::atan(dirToPlayer.x, dirToPlayer.y);
    int dirIndex = (int)round(4 * angle / 6.28318530718f + 5) % 4;
    const char* const dirText = s_dirNames[dirIndex];

    // Set animation based on state, regardless of fight phase
    std::string baseAnimName;
    switch (m_state)
    {
        case State::SIT:
            baseAnimName = "ThroneSitEyesClosed";
            break;
        case State::SUMMONING:
            break;
        case State::DEFLECT:
            break;
        case State::APPROACH:
        case State::STRAFE:
        case State::DODGE:
        case State::LEAP_ATTACK:
            baseAnimName = m_pAxeCollider ? "WalkNoAxe" : "Walk";
            break;
        case State::AXE_ATTACK:
            baseAnimName = "Growl";
            break;
        case State::SEARCHING:
            baseAnimName = "Crawl";
            break;
        case State::FIRE_BREATH_ATTACK:
            baseAnimName = m_fireBreathWindupTimer <= 0.0f ? "FireBreathLoop" : "FireBreath";
            break;
        case State::CHARGE_ATTACK:
            baseAnimName = m_chargeWindupTimer <= 0.0f ? "ChargeLoop" : "Charge";
            break;
        case State::IDLE:
        case State::PULL:
        case State::STUNNED:
            baseAnimName = "Crawl";
            break;
        case State::TAUNT:
            break;
        case State::DEAD:
            break;
    }

    // Add direction to base anim name
    if (m_state != State::SIT)
    {
        baseAnimName += dirText;
    }

    if (baseAnimName != "")
    {
        m_pAnimSprite->SetAnimation(baseAnimName);
    }
}

void BossController::RenderHealthBar(float delta)
{
    // Position bar at the bottom center of the screen
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const float barWidth = displaySize.x * 0.75f;
    const float barHeight = 48.0f;
    ImVec2 basePos = ImVec2(displaySize.x / 2 - barWidth / 2, displaySize.y - barHeight * 2);

    // Reset damage flash timer after 1 second
    if (m_damageFlashTimer.Elapsed() > 1.0f)
    {
        m_damageFlashTimer.Reset();
    }

    // Renders rainbow text with ImGui
    std::function<void(const char*, bool)> Rainbowify = [](const char* text, bool color) {
        float time = ImGui::GetTime() * 2.0f; // Adjust speed
        float offset = 0.0f;
        for (const char* c = text; *c != '\0'; c++) {
            float r = 0.5f + 0.5f * std::sin(time + offset);
            float g = 0.5f + 0.5f * std::sin(time + offset + 2.0f);
            float b = 0.5f + 0.5f * std::sin(time + offset + 4.0f);
            ImGui::TextColored(color ? ImVec4(r, g, b, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%.1s", c);
            ImGui::SameLine(0.0f, 0.0f);
            offset += 0.2f;
        }
    };

    // Render the health bar at the top-center of the screen
    auto* healthComponent = GetGameObject()->GetComponent<HealthComponent>();
    if (healthComponent)
    {
        // Flash health when damaged
        float colorCoefficient = m_damageFlashTimer.IsRunning() ? 1.0f - m_damageFlashTimer.Elapsed() : 0.0f;

        // Update health color
        ImVec4 healthColor = ImVec4(1.0f, colorCoefficient, colorCoefficient, 1.0f);

        // Render previous health fraction underneath to indicate damage taken
        m_prevHealthFraction += (healthComponent->GetHealth() / healthComponent->GetMaxHealth() - m_prevHealthFraction) * delta * 4.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

        // Draw health bar
        ImGui::SetNextWindowPos(basePos);
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##BossHealthBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
        ImGui::ProgressBar(healthComponent->GetHealth() / healthComponent->GetMaxHealth(), ImVec2(-1.0f, -1.0f), "");
        ImGui::PopStyleColor();
        ImGui::End();

        // Draw gradual effect of health bar
        ImGui::SetNextWindowPos(basePos);
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##BossHealthBar2", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
        ImGui::ProgressBar(m_prevHealthFraction, ImVec2(-1.0f, -1.0f));
        ImGui::PopStyleColor();
        ImGui::End();

        // Render text
        ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y - barHeight * 0.65f));
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##BossHealthBarText", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::SetWindowFontScale(2.5f);
        ImGui::SetCursorPos(ImVec2(barWidth / 2 - ImGui::CalcTextSize("The Minotaur").x / 2, 0.0f));
        Rainbowify("The Minotaur", false);
        ImGui::SetCursorPos(ImVec2(barWidth / 2 - ImGui::CalcTextSize("The Minotaur").x / 2 + 3.0f, 3.0f));
        Rainbowify("The Minotaur", true);
        ImGui::End();

        ImGui::PopStyleVar(2);
    }
}

void BossController::OnDamageEvent(const DamageEvent& event)
{
    // React to damage and reset damage flash timer
    if (event.m_pDamagedObject == GetGameObject())
    {
        m_damageFlashTimer.Restart();
    }
}

// <----------------- PHASE 1 METHODS ----------------->

void BossController::EnterPhase1()
{
    m_phase = FightPhase::PHASE_1;
    m_state = State::SIT;

    // TODO: Phase 1 initialization logic
    if (m_pHealth){
        m_pHealth->SetActive(false);
    }

    if (m_pVelocity){
        m_pVelocity->SetKnockbackEnabled(false);
    }

    DDACalculator* dda = DDACalculator::GetInstance();
    if (!dda)
    {
        wolf::Error("BossController: DDACalculator instance is null!");
        return;
    }
}

void BossController::UpdatePhase1(float delta)
{
    // Handle forcefield and knockback
    HandleForcefield(delta);
    HandleKnockBackCollision(delta);
    CheckWaveProgress(delta);
    CheckEnemyWaveHealth();
    RenderImGui();

    // **Trigger Wave 1 if the player attacks near the boss**
    if (!m_waveActive && m_pPlayerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING)
    {
        // wolf::Log("BossController: Player attacked, starting Wave 1...");
        StartWave();
    }
}

void BossController::CleanupPhase1()
{
    // Clear any remaining enemies from the scene
    for (const auto& enemyID : m_enemyIDs)
    {
        auto enemy = GetGameObject()->GetScene().GetObject(enemyID);
        if (enemy)
        {
            enemy->Delete();
        }
    }
    m_enemyIDs.clear();

    // Reset wave-related variables
    m_currentWave = 0;
    m_waveActive = false;
    m_waveTransitionTimer = 0.0f;

    // Log cleanup
    // wolf::Log("BossController: Phase 1 cleanup complete.");
}

void BossController::HandleForcefield(float delta)
{
    if (!m_pPlayerObject || !m_pTransform || !m_pPlayerController) return;

    glm::vec2 bossPos = m_pTransform->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    float distance = glm::distance(playerPos, bossPos);

    VelocityComponent* pPlayerVelocity = m_pPlayerObject->GetComponent<VelocityComponent>();
    glm::vec2 currentVelocity = pPlayerVelocity->GetVelocity();

    // Ignore slowdown if rolling
    if (m_pPlayerController->GetPlayerAction() == PlayerController::PlayerAction::ROLLING)
    {
        return;
    }

    // Define slowdown effect range
    float slowdownStart = m_forcefieldRadius * 0.9f; // Start slowing at 90% of forcefield radius
    float fullStopDistance = m_forcefieldRadius * 0.25f; // Full stop effect closer at 25%

    if (distance < m_forcefieldRadius)
    {
        // Gradual slowdown scaling with distance
        float slowdownFactor = glm::smoothstep(fullStopDistance, slowdownStart, distance);
        slowdownFactor = glm::clamp(slowdownFactor, 0.05f, 1.0f);  

        // Apply pushback resistance if moving toward the boss
        glm::vec2 directionToBoss = glm::normalize(bossPos - playerPos);
        float forwardSpeed = glm::dot(currentVelocity, directionToBoss);
        if (forwardSpeed > 0)  
        {
            currentVelocity -= directionToBoss * (forwardSpeed * 0.3f);
        }

        // Apply slowdown
        glm::vec2 newVelocity = currentVelocity * slowdownFactor;
        pPlayerVelocity->SetVelocity(newVelocity);

        // **Trigger Wave 1 if not already active**
        if (!m_waveActive)
        {
            // wolf::Log("BossController: Player entered forcefield, starting Wave 1...");
            StartWave();
            m_waveActive = true;

        }
    }
}
void BossController::HandleKnockBackCollision(float delta)
{
    if (!m_pPlayerObject || !m_pCollider || !m_pPlayerController || !m_pPlayerObject->HasAll<ColliderComponent>())
        return;

    ColliderComponent* pPlayerCollider = m_pPlayerObject->GetComponent<ColliderComponent>();

    // 1. Check if the player physically collides with the boss
    if (ColliderManager::StaticMethodIsColliding(*m_pCollider, *pPlayerCollider, delta))
    {
        glm::vec2 bossPos = m_pTransform->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 knockbackDirection = glm::normalize(playerPos - bossPos);
        float knockbackForce = 11000.0f;

        m_pPlayerObject->GetComponent<VelocityComponent>()->ApplyKnockback(knockbackDirection, knockbackForce);
        return; // No need to continue after this
    }

    // 2. Check if the player is attacking with a non-bow weapon and is close to the boss
    if (m_pPlayerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING)
    {
        // Retrieve the player's current weapon
        WeaponItem* pWeapon = dynamic_cast<WeaponItem*>(m_pPlayerController->GetHeldWeapon());
        if (pWeapon && !pWeapon->GetHasProjectiles()) // Ensure the weapon is not a bow
        {
            glm::vec2 bossPos = m_pTransform->GetGlobalPosition();
            glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            float distance = glm::distance(playerPos, bossPos);

            // Apply knockback if the player is within a close range to the boss
            const float meleeRange = 50.0f; // Define the melee attack range
            if (distance <= meleeRange)
            {
                glm::vec2 knockbackDirection = glm::normalize(playerPos - bossPos);
                float knockbackForce = 11000.0f;

                m_pPlayerObject->GetComponent<VelocityComponent>()->ApplyKnockback(knockbackDirection, knockbackForce);
            }
        }
    }
}

void BossController::CheckWaveProgress(float delta)
{
    if (!m_waveActive) return;

    // Wait for wave transition timer
    if (m_remainingEnemies == 0)
    {
        if (m_waveTransitionTimer > 0.0f)
        {
            m_waveTransitionTimer -= delta;
            return;
        }

        // Move to next wave
        if (m_currentWave < 3)
        {
            m_currentWave++;  // Increment the wave
            SpawnWave(m_currentWave);
        }
        else
        {
            // All waves completed
            m_waveActive = false; // Mark waves as inactive
            // wolf::Log("All waves cleared! Transitioning to Phase 2...");
            CleanupPhase1(); // Cleanup Phase 1 specific elements
            EnterPhase2();
        }
    }
}

// Trigger Wave 1 when player approaches the boss
void BossController::StartWave()
{
    if (m_waveActive) return;  // Prevent duplicate wave starts
    SpawnWave(1);
}

bool BossController::IsValidSpawnTile(glm::ivec2 tilePos)
{
    Tile::type tile = m_pLabyrinthManager->GetTile(tilePos.x, tilePos.y);
    
    bool isValid = (tile == Tile::BorderedGrass || tile == Tile::Bricks ||
                    tile == Tile::FloorSmallSquares || tile == Tile::FloorSmallSquaresGold ||
                    tile == Tile::FloorSpiralGold || tile == Tile::FloorSpiral ||
                    tile == Tile::FloorSquareGold || tile == Tile::FloorSquare ||
                    tile == Tile::Grass);

    // wolf::Log("Checking tile at [%d, %d]: Type %d -> %s", tilePos.x, tilePos.y, tile, isValid ? "Valid" : "Invalid");

    return isValid;
}


void BossController::SpawnWave(int waveIndex)
{
    m_currentWave = waveIndex;
    m_waveActive = true;
    m_waveTransitionTimer = 2.0f;
    m_enemyIDs.clear();

    if (!m_pLabyrinthManager)
    {
        wolf::Error("BossController: LabyrinthManager not set!");
        return;
    }

    // Load enemy data
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    EnemyData harpyData = loader.LoadEnemyData("harpy");
    EnemyData gorgonData = loader.LoadEnemyData("gorgon");

    // Scene reference
    wolf::Scene& scene = GetGameObject()->GetScene();

    // Create builders
    MinitaurBuilder minitaurBuilder(scene);
    HarpyBuilder harpyBuilder(scene);
    GorgonBuilder gorgonBuilder(scene);

    // Define how many enemies per wave
    int minitaurs = 0, gorgons = 0, harpies = 0;
    switch (waveIndex)
    {
        case 1: minitaurs = 4; harpies = 2; break;
        case 2: gorgons = 2; harpies = 2; break;
        case 3: minitaurs = 15; break; // Swarm of minitaurs inside the bossfight room
    }

    // Get boss position
    glm::vec2 bossPos = m_pTransform->GetGlobalPosition();
    glm::ivec2 bossTilePos = m_pLabyrinthManager->GetTilePosition(bossPos);

    // If first wave, determine and save spawn locations
    static std::vector<glm::vec2> savedMinitaurSpawns;
    static std::vector<glm::vec2> savedHarpySpawns;
    
    // Define Minitaur spawn locations (grouped left & right)
    if (waveIndex == 1 || savedMinitaurSpawns.empty())
    {
        savedMinitaurSpawns.clear();
        glm::ivec2 leftGroupStart = bossTilePos + glm::ivec2(-4, -3);
        glm::ivec2 rightGroupStart = bossTilePos + glm::ivec2(4, -3);

        for (int i = 0; i < minitaurs; i++)
        {
            glm::ivec2 spawnTile = (i % 2 == 0) ? leftGroupStart + glm::ivec2(i, 0)
                                                : rightGroupStart + glm::ivec2(i, 0);
            glm::vec2 spawnPos = m_pLabyrinthManager->GetWorldPosition(spawnTile);
            if (IsValidSpawnTile(spawnTile)) savedMinitaurSpawns.push_back(spawnPos);
        }
    }

    // Define Harpy spawn locations (split left & right)
    if (waveIndex == 1 || savedHarpySpawns.empty())
    {
        savedHarpySpawns.clear();
        glm::ivec2 leftHarpy = bossTilePos + glm::ivec2(-6, -7);
        glm::ivec2 rightHarpy = bossTilePos + glm::ivec2(6, -7);

        if (IsValidSpawnTile(leftHarpy))
            savedHarpySpawns.push_back(m_pLabyrinthManager->GetWorldPosition(leftHarpy));
        if (IsValidSpawnTile(rightHarpy))
            savedHarpySpawns.push_back(m_pLabyrinthManager->GetWorldPosition(rightHarpy));
    }

    // Spawn Minitaurs (use saved positions for later waves)
    if (waveIndex != 3) // Normal spawning for waves 1 & 2
    {
        for (size_t i = 0; i < savedMinitaurSpawns.size() && i < minitaurs; i++)
        {
            auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, savedMinitaurSpawns[i]);
            minitaur.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
            m_enemyIDs.insert(minitaur.GetID());
        }
    }
    else // Special spawning for **Wave 3** (random minitaurs inside bossfight room)
    {
        int spawned = 0;
        while (spawned < minitaurs)
        {
            glm::vec2 spawnPos = GetRandomValidSpawnPosition();
            if (spawnPos != glm::vec2(-1, -1)) // Ensure valid position
            {
                auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, spawnPos);
                minitaur.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
                minitaur.GetComponent<VelocityComponent>()->ApplyKnockback(glm::vec2(2.0f,0.0f),1.0f);
                m_enemyIDs.insert(minitaur.GetID());
                spawned++;
            }
        }
    }

    // Spawn Harpies (use saved positions for later waves)
    for (size_t i = 0; i < savedHarpySpawns.size() && i < harpies; i++)
    {
        auto& harpy = harpyBuilder.BuildHarpy(harpyData, savedHarpySpawns[i]);
        harpy.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
        m_enemyIDs.insert(harpy.GetID());
    }

    // Define Gorgon spawn locations **only for wave 2+**
    if (waveIndex == 2 || waveIndex == 3)
    {
        glm::ivec2 leftGorgon = bossTilePos + glm::ivec2(-6, -5);
        glm::ivec2 rightGorgon = bossTilePos + glm::ivec2(6, -5);

        std::vector<glm::vec2> gorgonSpawns;
        if (IsValidSpawnTile(leftGorgon))
            gorgonSpawns.push_back(m_pLabyrinthManager->GetWorldPosition(leftGorgon));
        if (IsValidSpawnTile(rightGorgon))
            gorgonSpawns.push_back(m_pLabyrinthManager->GetWorldPosition(rightGorgon));

        for (size_t i = 0; i < gorgonSpawns.size() && i < gorgons; i++)
        {
            auto& gorgon = gorgonBuilder.BuildGorgon(gorgonData, gorgonSpawns[i]);
            gorgon.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
            m_enemyIDs.insert(gorgon.GetID());
        }
    }

    // Update remaining enemy count
    m_remainingEnemies = m_enemyIDs.size();
}




void BossController::CheckEnemyWaveHealth()
{
     if (!m_waveActive) return;
    // Iterate through a copy of the set to avoid modification during iteration
    std::vector<int> toRemove;

    for (int enemyID : m_enemyIDs)
    {
        wolf::GameObject* pEnemy = GetGameObject()->GetScene().GetObject(enemyID);

        // If the object doesn't exist, it has been deleted
        if (!pEnemy)
        {
            toRemove.push_back(enemyID); // Mark for removal
            m_remainingEnemies--; // Decrement the remaining enemy count
        }
    }

    // Remove all deleted enemies from the set
    for (int enemyID : toRemove)
    {
        m_enemyIDs.erase(enemyID);
    }
}

glm::vec2 BossController::GetRandomValidSpawnPosition()
{
    if (!m_pLabyrinthManager || !m_pTransform)
    {
        wolf::Error("BossController: LabyrinthManager or Transform2D is null!");
        return glm::vec2(-1, -1);
    }

    // **Get the boss's tile position**
    glm::vec2 bossWorldPos = m_pTransform->GetGlobalPosition();
    glm::ivec2 bossTilePos = m_pLabyrinthManager->GetTilePosition(bossWorldPos);

    // wolf::Log("BossController: Boss at tile position: [%d, %d]", bossTilePos.x, bossTilePos.y);

    // **Find the boss's room**
    std::optional<LabyrinthManager::RoomData> roomDataOpt = m_pLabyrinthManager->GetRoom(bossTilePos);
    if (!roomDataOpt.has_value())
    {
        // wolf::Warning("BossController: No room found for boss tile position!");
        return glm::vec2(-1, -1);
    }

    LabyrinthManager::RoomData room = roomDataOpt.value();
    glm::ivec2 roomMin = room.m_bounds.m_origin;
    glm::ivec2 roomMax = room.m_bounds.m_origin + room.m_bounds.m_size;

    // wolf::Log("BossController: Room bounds - Min: [%d, %d], Max: [%d, %d]", 
    //           roomMin.x, roomMin.y, roomMax.x, roomMax.y);

    // **Try generating a valid spawn point within the room**
    for (int attempts = 0; attempts < 10; ++attempts)
    {
        glm::ivec2 spawnTilePos(m_rng.NextInt(roomMin.x, roomMax.x), m_rng.NextInt(roomMin.y, roomMax.y));

        if (IsValidSpawnTile(spawnTilePos))
        {
            //Add a half-tile offset to center the spawn position within the tile
            glm::vec2 worldPos = m_pLabyrinthManager->GetWorldPosition(spawnTilePos) + glm::vec2(48.0f, 48.0f);
            // wolf::Log("BossController: Spawned enemy at tile [%d, %d], world [%f, %f]", 
            //           spawnTilePos.x, spawnTilePos.y, worldPos.x, worldPos.y);
            return worldPos;
        }
    }

    // wolf::Warning("BossController: No valid spawn tile found after 10 attempts!");
    return glm::vec2(-1, -1);
}


void BossController::RenderImGui()
{
    if (!m_waveActive) return; // Only render during waves

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Glow effect (static colors for a clean and consistent look)
    ImVec4 waveGlow = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Bright red glow for wave text
    ImVec4 enemiesGlow = ImVec4(0.3f, 0.9f, 1.0f, 1.0f); // Bright cyan glow for enemies text

    // Add some subtle gradient-like styles for fun
    ImVec4 backgroundColor = ImVec4(0.0f, 0.0f, 0.2f, 0.6f); // Dark blue with transparency
    ImVec4 borderColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);     // Bright orange for a striking border

    // Push custom styles
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f); // Thicker border for emphasis
    ImGui::PushStyleColor(ImGuiCol_WindowBg, backgroundColor);
    ImGui::PushStyleColor(ImGuiCol_Border, borderColor);

    // **Wave Text**
    ImVec2 wavePos = ImVec2(displaySize.x * 0.5f, 40.0f);  // Top-center of the screen
    ImGui::SetNextWindowPos(wavePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("WaveText", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushStyleColor(ImGuiCol_Text, waveGlow);
    ImGui::Text("Wave %d", m_currentWave);
    ImGui::PopStyleColor();
    ImGui::End();

    // **Enemies Remaining Text**
    ImVec2 enemiesPos = ImVec2(displaySize.x * 0.5f, 80.0f);  // Slightly below wave text
    ImGui::SetNextWindowPos(enemiesPos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("EnemiesText", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushStyleColor(ImGuiCol_Text, enemiesGlow);
    ImGui::Text("Enemies Remaining: %d", m_remainingEnemies);
    ImGui::PopStyleColor();
    ImGui::End();

    // Pop styles to clean up
    ImGui::PopStyleColor(2); // Pop WindowBg and Border colors
    ImGui::PopStyleVar(3);   // Pop WindowPadding, WindowRounding, and FrameBorderSize
}



// <----------------- PHASE 2 METHODS ----------------->

void BossController::EnterPhase2()
{
    m_phase = FightPhase::PHASE_2;
    m_state = State::APPROACH;
    m_nextAttackTimer.Restart();
    m_axeAttackTimer.Reset();
    m_pHealth->SetActive(true);
    m_pVelocity->SetKnockbackEnabled(true);

    // Create the shadow sprite object
    if (m_pShadowObject) m_pShadowObject->Delete();
    m_pShadowObject = &GetGameObject()->GetScene().CreateObject2D();
    GetGameObject()->AddChild(*m_pShadowObject);

    auto& transform = *m_pShadowObject->GetComponent<wolf::Transform2D>();
    transform.SetPosition(glm::vec2(0.0f, -m_shadowDistance));

    // Add the sprite
    wolf::Sprite2D& sprite = m_pShadowObject->AddComponent<wolf::Sprite2D>("data/textures/boss_shadow.png");
    sprite.SetOriginToCenterOfTexture();
}

void BossController::UpdatePhase2(float delta)
{
    // Timing variables
    static float nextStrafeSwap = 1.0f;
    static float nextAttackTime = 2.0f;

    // Query player spatial info
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 toPlayer = playerPos - m_pTransform->GetGlobalPosition();
    float distToPlayer = glm::length(toPlayer);
    glm::vec2 dirToPlayer = glm::normalize(toPlayer);
    
    // Protect against nan
    if (glm::isnan(dirToPlayer.x)) dirToPlayer.x = 0.0f;
    if (glm::isnan(dirToPlayer.y)) dirToPlayer.y = 0.0f;

    // Get rotated direction
    glm::mat4 rotation = glm::rotate(glm::radians(m_strafeClockwise ? 90.0f : -90.0f), glm::vec3(0, 0, 1));
    glm::vec2 rotated = glm::vec2(rotation * glm::vec4(dirToPlayer.x, dirToPlayer.y, 0, 1));

    // Query player controller state
    bool playerAttacking = m_pPlayerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;

    wolf::Log("State: ", (int)m_state);

    // Update axe if it exists
    if (m_pAxeCollider)
    {
        // Play whoosh wfx
        if (m_whooshTimer.Elapsed() >= 0.32f)
        {
            wolf::Audio::Play("data/sounds/sfx_axe_whoosh.wav", 0.4f);
            m_whooshTimer.Restart();
        }

        // Calculate vector from axe to player
        glm::vec2 axePos = m_pAxeCollider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 axeToPlayer = glm::normalize(playerPos - axePos);

        // Protect against nan
        if (glm::isnan(axeToPlayer.x)) axeToPlayer.x = 0.0f;
        if (glm::isnan(axeToPlayer.y)) axeToPlayer.y = 0.0f;
        
        // Check for player collision
        auto* pPlayerCollider = m_pPlayerObject->GetComponent<ColliderComponent>();
        if (pPlayerCollider && 
            pPlayerCollider->IsHurtbox() &&
            m_pPlayerController->GetPlayerAction() != PlayerController::PlayerAction::ROLLING && 
            ColliderManager::StaticMethodIsColliding(*m_pAxeCollider, *pPlayerCollider, delta))
        {
            auto* pPlayerHealth = m_pPlayerObject->GetComponent<HealthComponent>();
            auto* pPlayerVel = m_pPlayerObject->GetComponent<VelocityComponent>();
            if (pPlayerHealth && pPlayerVel)
            {
                pPlayerHealth->Damage(m_axeAttackDamage);
                pPlayerVel->ApplyKnockback(axeToPlayer, 4500.0f);
                wolf::Audio::Play("data/sounds/sfx_heavy_hit.wav", 0.5f);
            }
        }

        // Collect axe and return to state
        if (m_axeAttackTimer.Elapsed() > 1.0f && glm::distance(axePos, m_pTransform->GetGlobalPosition()) < 128.0f)
        {
            m_pAxeCollider->GetGameObject()->Delete();
            m_pAxeCollider = nullptr;
            m_axeAttackTimer.Reset();
            m_axeSummoned = false;
            m_pAnimSprite->SetTint(glm::vec3(1.0f));
            m_whooshTimer.Reset();

            // Only change to approach if we are not in the middle of a leap attack
            if (m_state != State::LEAP_ATTACK)
            {
                m_state = State::APPROACH;
                m_nextAttackTimer.Restart();
            }
            else
            {
                m_nextAttackTimer.Reset();
            }
        }
    }

    // Defines a lambda for handling attack choice logic in phase 2
    std::function<void()> ChooseAttack = [&]
    {
        if (m_pAxeCollider)
        {
            // Only the leap attack is possible, do it
            // NOTE: This can infinite chain if the axe is not picked up! (intentional)
            StartLeapAttack();
        }
        else
        {
            // Ensure chains can't exceed 3
            if (m_attackChain == 3)
            {
                // Ensure we choose the attack NOT previously done
                m_prevAttack == 0 ? StartLeapAttack() : StartAxeAttack();
            }
            else
            {
                // Either attack is possible, randomly choose
                m_rng.FlipCoin() ? StartAxeAttack() : StartLeapAttack();
            }
        }
    };

    // Update based on state
    switch (m_state)
    {
        case State::APPROACH:

            // Handle attack logic
            if (m_nextAttackTimer.Elapsed() > nextAttackTime)
            {
                ChooseAttack();
                nextAttackTime = m_rng.NextFloat(0.5f, 3.0f);
                break;
            }

            // Normal approach logic
            if (distToPlayer > m_maxDistToPlayer)
            {
                // Approach player
                m_pVelocity->SetVelocity(dirToPlayer * m_chaseSpeed);
            }
            else if (distToPlayer < m_minDistToPlayer)
            {
                // Back away from player
                m_pVelocity->SetVelocity(-dirToPlayer * m_chaseSpeed);
            }
            else
            {
                // Switch to strafe state if within proper range
                m_state = State::STRAFE;
                m_strafeSwapTimer.Restart();
            }

            break;
        
        case State::STRAFE:
        {
            // Randomly change strafe direction
            if (!m_strafeSwapTimer.IsRunning())
            {
                m_strafeSwapTimer.Restart();
                nextStrafeSwap = m_rng.NextFloat(0.5f, 5.0f);
            }
            if (m_strafeSwapTimer.Elapsed() >= nextStrafeSwap)
            {
                m_strafeClockwise = !m_strafeClockwise;
                m_strafeSwapTimer.Reset();
            }
            
            // Move along strafe direction
            m_pVelocity->SetVelocity(rotated * m_strafeSpeed);

            // Switch back to approach if necessary
            if (distToPlayer > m_maxDistToPlayer || distToPlayer < m_minDistToPlayer)
            {
                m_state = State::APPROACH;
                break;
            }

            // Handle attack logic
            if (m_nextAttackTimer.Elapsed() > nextAttackTime)
            {
                ChooseAttack();
                nextAttackTime = m_rng.NextFloat(0.5f, 3.0f);
                break;
            }

            // Dodge player attack
            if (playerAttacking)
            {
                DodgePlayerAttack(dirToPlayer);
                break;
            }

            break;
        }

        case State::DODGE:

            // Return to approach state
            if (m_dodgeTimer.Elapsed() > 0.6f)
            {
                m_state = State::APPROACH;
                m_dodgeTimer.Reset();
            }

            break;
        
        case State::AXE_ATTACK:

            if (!m_axeSummoned)
            {
                // Spawn projectile
                if (m_axeAttackTimer.Elapsed() > 1.0f)
                {
                    // Update timer and flag
                    m_nextAttackTimer.Restart();
                    m_axeAttackTimer.Restart();
                    m_axeSummoned = true;
                    m_pAnimSprite->SetTint(glm::vec3(1.0f));

                    // Create the axe object
                    auto& axe = GetGameObject()->GetScene().CreateObject2D();
                    auto& transform = *axe.GetComponent<wolf::Transform2D>();
                    transform.SetPosition(m_pTransform->GetGlobalPosition());
                    transform.SetScale(glm::vec2(3.0f));
                    auto& velocity = axe.AddComponent<VelocityComponent>();
                    velocity.SetVelocity(dirToPlayer * 640.0f);
                    auto& sprite = axe.AddComponent<AnimatedSprite2D>("data/axe_spin_anim_init.yaml");
                    sprite.SetOriginToCenterOfFrame();
                    auto& homing = axe.AddComponent<HomingComponent>(GetGameObject(), 8.0f, 0.1f);
                    m_pAxeCollider = &axe.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, false, false);
                    m_pAxeCollider->AddColliderBox(glm::vec2(224), glm::vec2(-112, 112));

                    // Change state
                    m_state = State::APPROACH;

                    wolf::Audio::Play("data/sounds/sfx_axe_whoosh.wav", 0.4f);
                    m_whooshTimer.Restart();
                }
            }
            else
            {
                // We shouldn't reach here anyway, but safety!
                m_state = State::APPROACH;
            }
            break;
        
        case State::LEAP_ATTACK:

            double elapsed = m_leapAttackTimer.Elapsed();

            // TODO: Huge bug present, divide-by-zero causing -nan position in transform during leap attack...

            if (elapsed < 1.0f)
            {
                // Get current position without altitude added
                const glm::vec2 posWithoutAlt = m_pTransform->GetGlobalPosition() - glm::vec2(0.0f, m_altitude);

                // Update altitude
                m_altitude = glm::mix(0.0f, 300.0f, elapsed);

                // Update tint for tell
                m_pAnimSprite->SetTint(glm::vec3(1.0f) * (float)(elapsed + 1.0f));

                // Move towards the player
                const glm::vec2 target = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition() + glm::vec2(0.0f, 32.0f);
                const glm::vec2 toPlayer = target - posWithoutAlt;
                glm::vec2 move = glm::normalize(toPlayer);

                // Protect against nan
                if (glm::isnan(move.x)) move.x = 0.0f;
                if (glm::isnan(move.y)) move.y = 0.0f;

                // Update transform manually
                m_pTransform->SetPosition((posWithoutAlt + move * delta * 450.0f) + glm::vec2(0.0f, m_altitude));

                // Update sprite scale
                m_pShadowObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(glm::mix(1.0f, 0.4f, elapsed)));
            }
            else if (elapsed < 1.25f)
            {
                // Get current position without altitude added
                const glm::vec2 posWithoutAlt = m_pTransform->GetGlobalPosition() - glm::vec2(0.0f, m_altitude);

                // Update altitude
                m_altitude = glm::mix(300.0f, 0.0f, (elapsed - 1.0f) * 4.0f);
                m_pTransform->SetPosition(posWithoutAlt + glm::vec2(0.0f, m_altitude));

                m_pAnimSprite->SetTint(glm::vec3(1.0f));

                // Update sprite scale
                m_pShadowObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(glm::mix(0.4f, 1.0f, (elapsed - 1.0f) * 2.0f)));
            }
            else if (elapsed < 2.0f)
            {
                if (!m_slamStun)
                {
                    // Slam has completed
                    m_slamStun = true;
                    m_altitude = 0.0f;
                    m_pCollider->SetActive(true);
                    m_pVelocity->SetVelocity(glm::vec2(0.0f));
                    m_nextAttackTimer.Reset();

                    // Reset shadow scale
                    m_pShadowObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1.0f));

                    // Damage the player and knockback
                    auto* pPlayerCollider = m_pPlayerObject->GetComponent<ColliderComponent>();
                    if (pPlayerCollider &&
                        ColliderManager::StaticMethodIsColliding(*m_pCollider, *pPlayerCollider, delta))
                    {
                        auto* pPlayerHealth = m_pPlayerObject->GetComponent<HealthComponent>();
                        auto* pPlayerVel = m_pPlayerObject->GetComponent<VelocityComponent>();
                        if (pPlayerHealth && pPlayerVel)
                        {
                            // Only damage if not rolling and a hurtbox is present
                            if (pPlayerCollider->IsHurtbox() &&
                                m_pPlayerController->GetPlayerAction() != PlayerController::PlayerAction::ROLLING)
                            {
                                pPlayerHealth->Damage(m_slamAttackDamage);
                            }

                            // ALWAYS knockback even if rolling
                            pPlayerVel->ApplyKnockback(dirToPlayer, 3000.0f);
                            
                            // TODO: Play slam sfx
                        }

                        // TODO: Shockwave effect
                    }
                }

                m_pAnimSprite->SetTint(glm::mix(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f), (elapsed - 1.25f) * 1.25f));
            }
            else
            {
                // Finally transition to approach after the stun wears off
                m_state = State::APPROACH;
                m_nextAttackTimer.Restart();
                m_leapAttackTimer.Reset();
                m_slamStun = false;
            }

            // Update shadow sprite offset
            m_pShadowObject->GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(0.0f, -m_shadowDistance - (m_altitude / 3)));

            break;
    }

    // Under half health, change to phase 3
    if (m_pHealth->GetHealth() <= m_maxHealth / 2)
    {
        // Cleanup
        if (m_pAxeCollider)
        {
            m_pAxeCollider->GetGameObject()->Delete();
            m_pAxeCollider = nullptr;
        }

        if (m_pShadowObject)
        {
            m_pShadowObject->Delete();
            m_pShadowObject = nullptr;
        }

        m_pAnimSprite->SetTint(glm::vec3(1.0f));
        EnterPhase3();
    }
}

void BossController::StartAxeAttack()
{
    // Can't spawn multiple axes!
    // NOTE: This check is needed as a failsafe for when the axe is out and 3 stomps have happened
    if (m_pAxeCollider) return;

    // Change state
    m_state = State::AXE_ATTACK;
    m_nextAttackTimer.Reset();
    m_axeAttackTimer.Restart();
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    wolf::Audio::Play("data/sounds/sfx_boss_growl.wav", 1.5f);

    // Update stats
    m_attackChain = (m_prevAttack == 0) ? m_attackChain + 1 : 1;
    m_prevAttack = 0;

}

void BossController::StartLeapAttack()
{
    // Change state
    m_state = State::LEAP_ATTACK;
    m_nextAttackTimer.Reset();
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    // m_pVelocity->ApplyKnockback(glm::vec2(0.0f, 1.0f), 1000.0f);

    // Disable collider and restart timer
    m_pCollider->SetActive(false);
    m_leapAttackTimer.Restart();

    // Update stats
    m_attackChain = (m_prevAttack == 1) ? m_attackChain + 1 : 1;
    m_prevAttack = 1;
}

void BossController::DodgePlayerAttack(const glm::vec2& dirToPlayer)
{
    // Change state
    m_state = State::DODGE;
    m_dodgeTimer.Restart();

    // Get randomly rotated direction
    glm::mat4 rotation = glm::rotate(glm::radians(m_rng.FlipCoin() ? 90.0f : -90.0f), glm::vec3(0, 0, 1));
    glm::vec2 rotated = glm::vec2(rotation * glm::vec4(dirToPlayer.x, dirToPlayer.y, 0, 1));

    // Dodge away from player for melee, dodge sideways for the bow
    WeaponItem* pWeapon = m_pPlayerController->GetHeldWeapon();
    m_dodgeDir = (pWeapon && pWeapon->GetWeaponType() == WeaponType::BOW) ? -rotated : -dirToPlayer;
    m_pVelocity->SetVelocity(m_dodgeDir * m_chaseSpeed * 1.4f);
}

// <----------------- PHASE 3 METHODS ----------------->

void BossController::EnterPhase3()
{   
    m_phase = FightPhase::PHASE_3;
    ChangeStatesPhase3(State::IDLE);
    m_pVelocity->SetKnockbackEnabled(false);

}

void BossController::UpdatePhase3(float delta)
{
    // TODO: Phase 3 update logic:
    // - Stand in place and search for player when in neutral (can only see forward, rotate around?)
    // - When player found, if close, do fire breath attack
    // - if far away, do charge attack
    if (m_pHealth->GetHealth() <= 0 && m_state != State::DEAD)
    {
        m_state = State::DEAD;
        wolf::Log("It may have been the Minotaur's labyrinth but Theseus the GOAT");
    }

    switch (m_state)
    {
        case State::DEAD:
        {
            break;
        }

        case State::CHARGE_ATTACK:
        {
            AttackCharge(delta);
            break;
        }

        case State::FIRE_BREATH_ATTACK:
        {
            AttackFireBreath(delta);
            break;
        }

        case State::IDLE:
        {
            Idle(delta);
            break;
        }

        case State::PULL:
        {
            Pull(delta);
            break;
        }

        case State::SEARCHING:
        {
            Search(delta);
            break;
        }

        case State::STUNNED:
        {
            Stunned(delta);
            break;
        }

        default:
        {
            break;
        }

        // TODO: Death animation + ending cutscene!
    }
}

// Resued from GorgonController
void BossController::MoveTowardsPlayer(float delta)
{
    if (!m_pPlayerObject || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Gorgon
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 

    // Calculate direction vector
    glm::vec2 direction = playerPos - thisPos;

    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_searchSpeed);

    } 
    else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void BossController::ChangeStatesPhase3(State p_state)
{
    // Return if state is not in phase 3
    if(p_state < State::SEARCHING)
    {
        return;
    }

    // End old state
    switch (m_state)
    {
        case State::CHARGE_ATTACK:
        {
            EndChargeAttack();
            break;
        }
        case State::FIRE_BREATH_ATTACK:
        {
            break;
        }
        case State::IDLE:
        {
            EndIdle();
            break;
        }
        case State::SEARCHING:
        {
            EndSearch();
            break;
        }
        default:
        break;
    }

    // Start new state
    switch (p_state)
    {
        case State::CHARGE_ATTACK:
        {
            StartChargeAttack();
            break;
        }
        case State::FIRE_BREATH_ATTACK:
        {
            StartFireBreathAttack();
            break;
        }
        case State::IDLE:
        {
            StartIdle();
            break;
        }
        case State::PULL:
        {
            StartPull();
            break;
        }
        case State::SEARCHING:
        {
            StartSearch();
            break;
        }
        case State::STUNNED:
        {
            StartStunned();
            break;
        }

        default:
        break;
    }

    m_state = p_state;
}

void BossController::StartSearch()
{
    wolf::RNG rng;
    m_searchTimer = rng.NextFloat(2.5f, 3.5f);
}

void BossController::Search(float delta)
{

    if(m_searchTimer <= 0.0f)
    {
        // Decide next attack
        int rngAtk = m_rng.NextInt(1, 10);
        // Pull
        if(rngAtk <= 5)
        {
            ChangeStatesPhase3(State::PULL);
            return;
        }
        // Charge
        else
        {
            ChangeStatesPhase3(State::CHARGE_ATTACK);
            return;
        }
        
    }
    else
    {
        m_searchTimer -= delta;
        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
        float playerDistance = glm::distance(thisPos, playerPos);

        // If player close enough
        if(playerDistance <= m_autoAttackRange)
        {
            // Decide next attack
            int rngAtk = m_rng.NextInt(1, 10);

            // Fire breath
            if(rngAtk <= 5)
            {
                ChangeStatesPhase3(State::FIRE_BREATH_ATTACK);
                return;
            }
            
            // Charge
            else
            {
                ChangeStatesPhase3(State::CHARGE_ATTACK);
                return;
            }
            
        }
        else
        {
            MoveTowardsPlayer(delta);
            return; 
        }
    }

}

void BossController::EndSearch()
{

}

void BossController::StartIdle()
{
    m_idleTimer = m_rng.NextFloat(1.0f, 2.0f);
}

void BossController::Idle(float delta)
{
    if(m_idleTimer <= 0.0f)
    {
        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
        float playerDistance = glm::length(thisPos - playerPos);

        // If player close enough
        if(playerDistance <= m_autoAttackRange)
        {
            // Decide next attack
            int rngAtk = m_rng.NextInt(1, 10);
            // Fire breath
            if(rngAtk <= 5)
            {
                ChangeStatesPhase3(State::FIRE_BREATH_ATTACK);
                return;
            }
            // Charge
            else
            {
                ChangeStatesPhase3(State::CHARGE_ATTACK);
                return;
            }
        }
        // If player is far away
        else
        {
            // Decide next action
            int rngAct = m_rng.NextInt(1, 10);
            if(rngAct < 5)
            {
                ChangeStatesPhase3(State::SEARCHING);
                return;
            }
            else
            {
                ChangeStatesPhase3(State::PULL);
                return;
            }
        }
    }
    else
    {
        // TODO: Add sprites that turn towards player
        m_idleTimer -= delta;
    }

}

void BossController::EndIdle()
{

}

void BossController::StartStunned()
{
    m_stunTimer = 0.5f;
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
}

void BossController::Stunned(float delta)
{
    if(m_stunTimer <= 0.0f)
    {
        if(m_chargeChainCount > 0)
        {
            ChangeStatesPhase3(State::CHARGE_ATTACK);
            return;
        }
        else
        {
            ChangeStatesPhase3(State::IDLE);
            return;
        }
    }
    else
    {
        m_stunTimer -= delta;
    }
}

void BossController::StartFireBreathAttack()
{
    m_fireBreathWindupTimer = m_fireBreathWindupTime;
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));

    m_fireBreathDuration = 7.0f;
    GetGameObject()->GetComponent<VelocityComponent>()->SetVelocity(glm::vec2(0.0f, 0.0f));
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    m_lastDirection = glm::normalize(playerPos - thisPos);

    wolf::Audio::Play("data/sounds/sfx_boss_growl.wav", 1.5f, -3000.0f);
}

void BossController::AttackFireBreath(float delta)
{
    // If windup expired
    if(m_fireBreathWindupTimer <= 0.0f)
    {
        // If fire breath expired, change state to search
        if(m_fireBreathDuration <= 0.0f)
        {
            ChangeStatesPhase3(State::IDLE);
            return;    
        }

        // Update attack timer & state
        m_fireBreathDuration -= delta;

        // Get data
        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 endPos = thisPos + m_lastDirection * m_fireBreathRange;

        // Add fire range indicator
        GLShapesRenderer::GetInstance()->AddLine({thisPos.x, thisPos.y, 1, 0, 0 , 1}, {endPos.x, endPos.y, 1, 0, 0 , 1});

        // If turning delay expired
        if(this->m_fireBreathTurningTimer >= this->m_fireBreathTurningDelay)
        {
            // Turn towards player
            TurnToPlayer(delta);
            
            // Get all tiles burnt
            std::vector<glm::ivec2> tiles = DDACalculator::GetInstance()->GetTraversedTiles(thisPos, endPos, true);

            // Add fire tiles
            for (glm::ivec2 tile : tiles)
            {
                TileFireManager::GetInstance()->AddFireTile(tile, 8.0f);
            }

            // Reset timer
            this->m_fireBreathTurningTimer = 0.0f;
        }
        else
        {
            this->m_fireBreathTurningTimer += delta;
        }
    }
    else
    {
        m_fireBreathWindupTimer -= delta;
        
        // If entering attack
        if(m_fireBreathWindupTimer <= 0.0f)
        {
            m_pAnimSprite->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
            wolf::Audio::Play("data/sounds/sfx_fire.wav", 0.55f);

        }
        // If still windup
        else
        {
            glm::vec3 currentTint = m_pAnimSprite->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_fireBreathWindupTime * 0.5f), 0.0f, 0.0f);
            m_pAnimSprite->SetTint(nextTint);
        }        
    }
}

void BossController::TurnToPlayer(float delta)
{
    // Get data
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
    float playerDistance = glm::length(thisPos - playerPos);
    
    if(playerDistance > 0.0f)
    {
        glm::vec2 playerDirection = glm::normalize(playerPos - thisPos);   
        
        if(m_lastDirection == playerDirection)
        {
            return;
        }
        
        // Calculate angle 
        float dotProduct = glm::dot(m_lastDirection, playerDirection);
        float cos = glm::clamp(dotProduct, -1.0f, 1.0f);
        float radAngle = glm::acos(cos);

        // If turning angle is smaller than cap
        if(std::abs(radAngle) <= std::abs(m_fireBreathTurningCapRadian))
        {
            m_lastDirection = playerDirection;
        }

        else
        {
            // Calculate rotation side
            float side = glm::cross(glm::vec3(m_lastDirection.x, m_lastDirection.y, 0), glm::vec3(playerDirection.x, playerDirection.y, 0)).z;
            glm::vec2 newDirection = glm::vec2(0.0f, 0.0f);

            float capSin = glm::sin(m_fireBreathTurningCapRadian);
            float capCos = glm::cos(m_fireBreathTurningCapRadian);
            // Left
            if(side >= 0.0f)
            {
                newDirection.x = m_lastDirection.x * capCos - m_lastDirection.y * capSin;
                newDirection.y = m_lastDirection.y * capCos + m_lastDirection.x * capSin;
            }
            // Right
            else
            {
                newDirection.x = m_lastDirection.x * capCos + m_lastDirection.y * capSin;
                newDirection.y = m_lastDirection.y * capCos - m_lastDirection.x * capSin;
            }

            m_lastDirection = newDirection;
        }
    }
}

void BossController::StartChargeAttack()
{
    m_chargeWindupTimer = m_chargeWindupTime;
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));

    // Reset chain count
    if(m_chargeChainCount <= 0)
    {
        m_chargeChainCount = m_rng.NextInt(1, 3);
    }

    m_chargeStompSFXTimer.Restart();
    wolf::Audio::Play("data/sounds/sfx_boss_growl.wav", 1.5f, 3000.0f);
}

void BossController::AttackCharge(float delta)
{
    // If windup expired
    if(m_chargeWindupTimer <= 0.0f)
    {
        // Play stomping sfx
        if (m_chargeStompSFXTimer.Elapsed() > 0.25f)
        {
            wolf::Audio::Play("data/sounds/sfx_stomp.wav", 0.7f, m_rng.NextFloat(-4000, 4000));
            m_chargeStompSFXTimer.Restart();
        }

        m_pVelocity->SetVelocity(glm::normalize(m_pVelocity->GetVelocity()) * m_chargeSpeed);

        // Get all colliders 
        for (auto&&[id, collider] : this->GetGameObject()->GetScene().Each<ColliderComponent>())
        {
            // If other collider is not the same collider, is active & is hitbox
            if (GetGameObject()->GetID() != id && collider.IsActive() && collider.IsHitbox())
            {
                // If colliders colliding
                if (ColliderManager::StaticMethodIsColliding(*m_pCollider, collider, delta))
                {
                    // If player, damge player
                    if(id == m_pPlayerObject->GetID())
                    {
                        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
                        glm::vec2 playerDirection = (playerPos - thisPos) == glm::vec2(0.0f) ? 
                                                                                                glm::vec2(1.0f, 0.0f):
                                                                                                glm::normalize(playerPos - thisPos);
                        m_pPlayerObject->GetComponent<HealthComponent>()->Damage(100.0f);
                        m_pPlayerObject->GetComponent<VelocityComponent>()->ApplyKnockback(playerDirection, m_chargeKnockbackForce);

                        wolf::Audio::Play("data/sounds/sfx_thud.wav", 1.1f);
                    }

                    // Change to STUNNED state
                    ChangeStatesPhase3(State::STUNNED);
                    return;
                }
            }
        }
    }
    else
    {
        // Update windup timer
        m_chargeWindupTimer -= delta;

        // If entering attack
        if(m_chargeWindupTimer <= 0.0f)
        {
            m_pAnimSprite->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));

            // Calculate celocity
            glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
            glm::vec2 velo = glm::normalize(playerPos - thisPos) * m_chargeSpeed;
            
            // Set components for charge attack
            m_pVelocity->SetVelocity(velo);
            m_pHoming->SetActive(true);
        }
        // If still windup
        else
        {
            glm::vec3 currentTint = m_pAnimSprite->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_chargeWindupTime * 0.5f));
            m_pAnimSprite->SetTint(nextTint);
        }
    }
}

void BossController::EndChargeAttack()
{
    m_pHoming->SetActive(false);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));

    if(m_chargeChainCount > 0)
    {
        m_chargeChainCount--;
    }

    m_chargeStompSFXTimer.Reset();
}

void BossController::StartPull()
{
    m_pullTimer = m_pullTime;
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
}

void BossController::Pull(float delta)
{
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 

    // Add pull indicator
    GLShapesRenderer::GetInstance()->AddLine({thisPos.x, thisPos.y, 1, 1, 0, 1}, {playerPos.x, playerPos.y, 1, 1, 0, 1});

    // if pull timer expired
    if(m_pullTimer <= 0.0f)
    {
        // Decide next attack
        int rngAtk = m_rng.NextInt(1, 10);
        // Fire breath
        if(rngAtk <= 5)
        {
            ChangeStatesPhase3(State::FIRE_BREATH_ATTACK);
            return;
        }
        // Charge
        else
        {
            ChangeStatesPhase3(State::CHARGE_ATTACK);
            return;
        }
    }
    // If still pulling time
    else
    {
        // uupdate timer
        m_pullTimer -= delta;

        // Pull if player is not rolling
        if(m_pPlayerController->GetPlayerAction() != PlayerController::PlayerAction::ROLLING)
        {
            glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            m_pPlayerObject->GetComponent<VelocityComponent>()->ApplyKnockback(glm::normalize(glm::vec2(thisPos - playerPos)), m_pullForce);
        }
    }
}