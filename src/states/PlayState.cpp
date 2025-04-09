//-----------------------------------------------------------------------------
// File:			PlayState.cpp
// Original Author:	Youssef Ashraf
// Modifications : D'Anyil Landry, Nguyễn Minh Nhật, Aurora Ryder
// ver 1.1
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------

#include "PlayState.h"
#include "PauseState.h"
#include "DialogueAndCutsceneState.h"
#include <imgui/imgui.h>

#include "../components/ChestInventoryComponent.h"
#include "../components/ColliderComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HomingComponent.h"
#include "../components/PlayerInventoryComponent.h"
#include "../components/AttackDamageComponent.h"
#include "../components/MerchantInventoryComponent.h"
#include "../components/DispensaryInventoryComponent.h"
#include "../components/StatusComponent.h"
#include "../components/TimedDestroyerComponent.h"
#include "../components/TrappedChestComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/ThrowableObjectComponent.h"
#include "../components/BoulderTrapComponent.h"
#include "../components/MonsterSpawnerComponent.h"
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"
#include "DDACalculator.h"
#include "GLShapesRenderer.h"
#include "PortalTileManager.h"
#include "Postprocessor.h"
#include "TileFireManager.h"
#include "../npcs/NPCBuilder.h"
#include "../components/NPCComponent.h"
#include <BossController.h>
#include <LightComponent.h>
#include <W_Audio.h>
#include <events/PauseEvent.h>
#include <glm/gtc/random.hpp>
#include <LightEvents.h>
#include <BoundedFluidSystem2D.h>

#include <W_BufferManager.h>

void PlayState::Enter()
{

    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the dialogue listener
    wolf::EventManager::AddListener<DialogueAndCutsceneEvent, PlayState, &PlayState::OnDialogueAndCutsceneTriggered>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::AddListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);
    wolf::EventManager::AddListener<GameWinEvent, PlayState, &PlayState::OnGameWinEvent>(*this);
    
 
    this->m_pColliderManager = new ColliderManager(&scene);
    m_particleSystem = new ParticleSystem2D();

    // Initialize the player object
    CreatePlayer();

    // Add the main camera as a child object of the player
    auto& cameraObj = scene.CreateObject2D();
    auto& camera = cameraObj.AddComponent<wolf::Camera2D>(m_pGameInstance->GetWidth(), m_pGameInstance->GetHeight());
    m_pPlayerObject->AddChild(cameraObj);
    camera.SetPosition(cameraObj.GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Set the light's default FBO size to be the camera viewport size
    LightComponent::SetDefaultFBOSize(camera.GetViewSize());

    // Create framebuffer & scene texture
    glm::vec2 viewSize = camera.GetViewSize();
    m_pFBO = wolf::BufferManager::CreateFrameBuffer(viewSize.x, viewSize.y, viewSize.x, viewSize.y);

    // Add the labyrinth manager and load the default config
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();
    m_pLabyrinthManager->m_pColliderManager = m_pColliderManager;
    m_pLabyrinthManager->LoadConfig("data/labyrinth_config.yaml");

    // Initialize managers that require the labyrinth manager seed
    auto& pathfindingManagerObject = scene.CreateObject2D();
    m_pPathfindingManager = &pathfindingManagerObject.AddComponent<PathfindingManager>(m_pLabyrinthManager);    
    auto& navMeshObj = scene.CreateObject2D();
    m_pNavMeshComponent = &navMeshObj.AddComponent<NavMeshComponent>();
    m_pNavMeshComponent->Init(m_pPathfindingManager);
    NPCBuilder::CreateInstance(&scene, m_pLabyrinthManager->GetSeed());
    ItemDropCreator::CreateInstance(&scene, m_pLabyrinthManager->GetSeed());

    // Actually generate the labyrinth from the given seed
    m_pLabyrinthManager->GenerateLabyrinth();

    GLShapesRenderer::CreateInstance();
    DDACalculator::CreateInstance(&scene);

    PortalTileManager::CreateInstance(m_pLabyrinthManager);
    TileFireManager::CreateInstance(m_pLabyrinthManager);
    TileFireManager::GetInstance()->SetPropagationActiveness(true);

    Postprocessor::CreateInstance(&scene);

    // Place the bossfight trigger
    const auto& rooms = m_pLabyrinthManager->GetRooms();
    for (const auto& room : rooms)
    {
        if (room.m_name != "Minotaur's Chamber") continue;

        // Store door tile locations
        m_bossRoomDoorTiles = room.m_doors;

        // Create trigger object and collider
        auto& object = scene.CreateObject2D();
        auto& collider = object.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, false, false);
        auto size = glm::vec2(room.m_bounds.m_size.x, room.m_bounds.m_size.y) * (float)(LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
        auto position = glm::vec2(room.m_bounds.m_origin.x, room.m_bounds.m_origin.y + room.m_bounds.m_size.y);
        position *= LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE;
        collider.AddColliderBox(size, position);
        object.AddComponent<TriggerComponent>(m_pColliderManager, TriggerType::SINGLE_USE, TriggerPurpose::BOSS);

        // Set the position to teleport the player to when the bossfight starts
        m_bossfightPlayerPos = m_pLabyrinthManager->GetWorldPosition(room.m_bounds.m_origin) + size * 0.5f;
        m_bossfightPlayerPos.y -= (size.y * 0.25f);

        // Spawn the Minotaur Boss
        auto& bossObject = scene.CreateObject2D();
        auto& controller = bossObject.AddComponent<BossController>();

        // Move boss to initial location
        auto* pBossTransform = bossObject.GetComponent<wolf::Transform2D>();
        pBossTransform->SetPosition(m_bossfightPlayerPos + glm::vec2(0.0f, size.y * 0.5f));
        pBossTransform->SetScale(glm::vec2(3.0f));

        // Then call init (uses location to access boss room)
        controller.Init();

        // Grab pointer to boss object
        m_pBoss = &bossObject;
        m_pGameInstance->GetSharedContext().RegisterEntity("Minotaur", m_pBoss->GetID());

        // Create other object groups
        m_pBossWalls = &scene.CreateObject2D();
        m_bossRoomOrigin= room.m_bounds.m_origin;
        m_bossRoomSize = room.m_bounds.m_size;
        break;
    }
    
    glm::vec2 playerPosition = m_pLabyrinthManager->GetSpawnLocation();
    wolf::GameObject& ariadne = CreateAriadneAndReturn(playerPosition);

    // Track all Minotaurs and their positions
    std::unordered_map<MinitaurController*, glm::vec2> minitaurPositions;

    // Find all "Basic Fight Rooms" and record Minotaurs' positions
    for (const auto& room : m_pLabyrinthManager->GetRooms())
    {
        if (room.m_name == "Basic Fight Rooms")
        {
            glm::vec2 roomCenter = m_pLabyrinthManager->GetWorldPosition(glm::vec2(room.m_bounds.m_origin)) +
                                   glm::vec2(room.m_bounds.m_size) * 0.5f;

            // Record all Minotaurs in the room
            for (auto&& [_, minitaurController] : m_pGameInstance->GetScene().Each<MinitaurController>())
            {
                glm::vec2 minitaurPos = glm::vec2(minitaurController.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
                float distanceToRoom = glm::distance(roomCenter, minitaurPos);

                // Ensure the Minotaur is within this room
                if (distanceToRoom < glm::length(glm::vec2(room.m_bounds.m_size)) * LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
                {
                    minitaurPositions[&minitaurController] = minitaurPos;
                }
            }
        }
    }

    if (minitaurPositions.empty())
    {
        wolf::Log("No Minotaurs found in Basic Fight Rooms!");
        return;
    }

    // Find the closest Minotaur to the player
    MinitaurController* closestMinitaur = nullptr;
    float closestDistanceToPlayer = std::numeric_limits<float>::max();

    for (const auto& [minitaurController, position] : minitaurPositions)
    {
        float distanceToPlayer = glm::distance(playerPosition, position);
        if (distanceToPlayer < closestDistanceToPlayer)
        {
            closestMinitaur = minitaurController;
            closestDistanceToPlayer = distanceToPlayer;
        }
    }

    if (!closestMinitaur)
    {
        // wolf::Log("Failed to find the closest Minotaur to the player!");
        return;
    }

    // Add a light to the player
    wolf::GameObject* pLightGO = &m_pGameInstance->GetScene().CreateObject2D();
    auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(0.45f, 0.37f, 0.18f, 0.75f), 125.0f, true);
    m_pPlayerObject->AddChild(*pLightGO);
    pLightComponent.Init();
    pLightGO->GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(0.0f, -5.0f));

    // Make Ariadne's light pink because I can (Aurora)
    ariadne.GetChildren().front()->GetComponent<LightComponent>()->SetColor(glm::vec4(1.0f, 0.41f, 0.70f, 0.75f));

    // Register the closest Minotaur in the shared context
    m_pGameInstance->GetSharedContext().RegisterEntity("Minitaur", closestMinitaur->GetGameObject()->GetID());
    m_pGameInstance->GetSharedContext().RegisterEntity("Dispensary", m_pLabyrinthManager->GetTheDispensaryObject());

    // Schedule her movement
    auto* transform = ariadne.GetComponent<wolf::Transform2D>();
    if (transform) {
        glm::vec2 newPosition = transform->GetGlobalPosition() + glm::vec2(100.0f, 100.0f);
        transform->SetPosition(newPosition);
    }
    wolf::EventManager::EnqueueEvent(DialogueAndCutsceneEvent("intro_sequence", "data/DialogueAndCutscenes.yaml"));

    // Queue up all of Ariadne's dialogue
    auto* ariadneNPCComp = ariadne.GetComponent<NPCComponent>();
    ariadneNPCComp->QueueDialogue("hello");
    ariadneNPCComp->QueueDialogue("traps");
    ariadneNPCComp->QueueDialogue("survivors");

    // Stop all audio and begin the maze music
    wolf::Audio::Stop();
    wolf::Audio::Play("data/sounds/bgm_maze.wav", 0.65f, 0.0f, 0.0f, false, true, 13.714f);

    // Now it's safe to register entities
    for (auto&& [_, minitaur] : m_pGameInstance->GetScene().Each<MinitaurController>())
    {
        m_pPathfindingManager->RegisterEntity(minitaur.GetGameObject()); 
    }
    
    // Now it's safe to register entities
    for (auto&& [_, gorgon] : m_pGameInstance->GetScene().Each<GorgonController>())
    {
        m_pPathfindingManager->RegisterEntity(gorgon.GetGameObject());
    }



    // Generate the NavMesh from the Labyrinth
    m_pNavMeshComponent->GenerateFromLabyrinth(m_pLabyrinthManager);

    m_gameCompletionTime.Start();
}

void PlayState::Exit()
{
    wolf::Audio::Stop();

    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueAndCutsceneEvent, PlayState, &PlayState::OnDialogueAndCutsceneTriggered>(*this);
    wolf::EventManager::RemoveListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);

    m_pPathfindingManager = nullptr;
    wolf::EventManager::RemoveListener<GameWinEvent, PlayState, &PlayState::OnGameWinEvent>(*this);

    // Delete managers
    delete this->m_pColliderManager;
    this->m_pColliderManager = nullptr;
    DDACalculator::DestroyInstance();
    GLShapesRenderer::DestroyInstance();

    
    PortalTileManager::DestroyInstance();
    TileFireManager::DestroyInstance();

    Postprocessor::DestroyInstance();

    ItemDropCreator::DestroyInstance();
    NPCBuilder::DestroyInstance();

    if (m_particleSystem)
    {
        delete m_particleSystem;
        m_particleSystem = nullptr;
    }
    
    wolf::BufferManager::DestroyBuffer(m_pFBO);

    m_pNavMeshComponent = nullptr;

}

void PlayState::Pause()
{
}

void PlayState::Resume()
{
}

void PlayState::Update(float delta)
{
    auto* pCamera = m_pGameInstance->GetScene().GetActiveCamera();
    if (pCamera && m_bossZoomTimer.IsRunning())
    {
        if (m_bossZoomTimer.Elapsed() < 2.0f)
        {
            // Smooth interpolation based on cosine
            float elapsedNormalized = m_bossZoomTimer.Elapsed() * 0.5f;
            float t = (1.0f - cos(3.1415926535f * elapsedNormalized)) / 2;
            pCamera->SetZoom(glm::mix(1.0f, 0.85f, t));
        }
        else
        {
            // Stop zoom and set value
            pCamera->SetZoom(0.85f);
            m_bossZoomTimer.Reset();
        }
    }
    
    // Push the pause state when 'Escape' is pressed
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        auto pc = m_pPlayerObject->GetComponent<PlayerController>();
        if(pc->GetPlayerAction() == PlayerController::PlayerAction::PLACING)
        {
            pc->SetAction(PlayerController::PlayerAction::NONE);
        }
        else
        {
            wolf::EventManager::TriggerEvent(PauseEvent(true));
            m_pStateManager->PushState(new PauseState(m_pStateManager, m_pGameInstance));
        }
    }

    // Update debug hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_DELETE))
    {
        // Toggle debug hotkeys for both us and the player
        m_debugHotkeys = !m_debugHotkeys;
        m_pPlayerObject->GetComponent<PlayerController>()->m_debugHotkeys = m_debugHotkeys;
    }

    if (m_debugHotkeys)
    {
        // Show debug hitboxes with backslash
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_BACKSLASH))
        {
            m_pGameInstance->GetScene().ToggleDebugDrawing();
        }

        // Toggle Labyrinth Manager GUI with the semicolon key
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_SEMICOLON)) 
            m_showLabyrinthManager = !m_showLabyrinthManager;
        
        // DEBUG: Teleport to bossfight
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT_SHIFT))
            m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetPosition(m_bossfightPlayerPos);
            
        if(wolf::Input::IsKeyJustDown(GLFW_KEY_F))
        {
            glm::ivec2 playerTilePos = m_pLabyrinthManager->GetTilePosition(m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
            TileFireManager::GetInstance()->AddFireTile(playerTilePos);
        }

        // Show the Labyrinth Manager debug GUI
        if (m_showLabyrinthManager) 
            m_pLabyrinthManager->ShowGUI();
        
        m_pNavMeshComponent->Update(delta);
    }

    // Cache the player's position
    auto playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Update fluid system components
    for (auto&&[_, system] : m_pGameInstance->GetScene().Each<BoundedFluidSystem2D>())
    {
        system.Update(delta);

        // Always allow the player to splash through fluid systems
        bool playerRolling = m_pPlayerObject->GetComponent<PlayerController>()->GetPlayerAction() == PlayerController::PlayerAction::ROLLING;
        if (playerRolling)
        {
            system.ApplyRadialForce(playerPos, 50.0f, 1000.0f * delta);
        }

        // Only play splash sfx if initial roll + colliding
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && system.Intersects(playerPos))
        {
            wolf::Audio::Play("data/sounds/sfx_liquid_splash.wav", 0.64f);
        }

        // TODO: This logic should be encapsulated somewhere else...
        // Update logic for applying damage to the player via fluid traps
        auto parent = system.GetGameObject()->GetParent();
        if (parent)
        {
            auto trigger = parent->GetComponent<TriggerComponent>();
            if (trigger)
            {
                switch (trigger->GetPurpose())
                {
                    case TriggerPurpose::POISON_TRAP:
                        if (system.Intersects(playerPos))
                        {
                            m_pPlayerObject->GetComponent<StatusComponent>()->AddStatusEffect(StatusComponent::StatusEffectType::POISONED, 5.0f);
                        }
                        break;
                    
                    case TriggerPurpose::LAVA_TRAP:
                        if (system.Intersects(playerPos))
                        {
                            m_pPlayerObject->GetComponent<StatusComponent>()->AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 5.0f);
                        }
                        break;
                    
                    default:
                        break;
                }
            }
        }

        // DEBUG: Show editor and break after updating one system
        // system.ShowEditor();
        // break;
    }
    
    // Update the labyrinth manager
    m_pLabyrinthManager->Update(delta);

    // Shake the camera
    if (m_cameraShakeTimer.IsRunning() && m_cameraShakeTimer.Elapsed() < 2.0f)
    {
        float intensity = 5.0f; // Shake intensity
        glm::vec2 shakeOffset = glm::vec2(
            m_pLabyrinthManager->m_rng.NextFloat(-intensity, intensity),
            m_pLabyrinthManager->m_rng.NextFloat(-intensity, intensity)
        );
        pCamera->SetPosition(pCamera->GetPosition() + shakeOffset);
    }

    // Handle fade to black over 3 seconds
    if (m_fadeToBlackTimer.Elapsed() < 3.0f)
    {
        // Gradually increase alpha over 3 seconds
        float alpha = glm::clamp(static_cast<float>(m_fadeToBlackTimer.Elapsed()) / 3.0f, 0.0f, 1.0f);
        RenderFadeOverlay(alpha);
    }
    else
    {
        // If fade is fully elapsed, keep it completely black
        RenderFadeOverlay(1.0f);
    }
    
    // Display completion message for 5 seconds
    if (m_completionMessageTimer.IsRunning() && m_completionMessageTimer.Elapsed() < 5.0f)
    {
        RenderTextCentered("You have completed Theseus in " + std::to_string(m_gameCompletionTime.Elapsed()), 5.0f);
    }

    // Show credits after message disappears
    if (m_showCreditsTimer.IsRunning() && m_showCreditsTimer.Elapsed() < 20.0f)
    {
        RenderCredits(delta);
    }

    // Don't bother updating most of the code if we're in the credits
    if (!m_showCreditsTimer.IsRunning())
    {
        PortalTileManager::GetInstance()->Update(delta);
        TileFireManager::GetInstance()->Update(delta);

        // Update timed destroyer components
        for (auto&& [_, TimedDestroyerComponent] : m_pGameInstance->GetScene().Each<TimedDestroyerComponent>())
        {
            TimedDestroyerComponent.Update(delta);
        }

        // INVENTORY TESTING
        auto* playerInventory = m_pPlayerObject->GetComponent<PlayerInventoryComponent>();
        if (playerInventory) {
            if (m_debugHotkeys)
            {
                if (wolf::Input::IsKeyJustDown(GLFW_KEY_0)) playerInventory->ToggleOpen();

                if (wolf::Input::IsKeyJustDown(GLFW_KEY_1)) {
                    ItemBase* pBoots = ItemCreator::CreateItem("The Floor is Lava Boots");
                    ItemBase* pDentedHelmet = ItemCreator::CreateItem("Dented Helmet");
                    ItemBase* pRustyChestplate = ItemCreator::CreateItem("Rusty Chestplate");
                    ItemBase* pCopperVambraces = ItemCreator::CreateItem("Copper Vambraces");
                    ItemBase* pKilt = ItemCreator::CreateItem("Kilt");
                    ItemBase* pTheezys = ItemCreator::CreateItem("Theezys");
                    ItemBase* pFauxLeatherGloves = ItemCreator::CreateItem("Faux-leather Gloves");
                    ItemBase* pLapisLazuliRing = ItemCreator::CreateItem("Lapis Lazuli Ring");
                    ItemBase* pPortal1 = ItemCreator::CreateItem("Portal");
                    ItemBase* pPortal2 = ItemCreator::CreateItem("Portal");

                ItemBase* pBow = ItemCreator::CreateItem("Old Bow");
                ItemBase* pSpear = ItemCreator::CreateItem("Spear");

                    ItemBase* pHealHeart = ItemCreator::CreateItem("Healing Heart");
                    ItemBase* pHurtHeart = ItemCreator::CreateItem("Hurting Heart");
                    ItemBase* pBurnHeart = ItemCreator::CreateItem("Burning Heart");
                    
                    playerInventory->AddItemOrDelete(pPortal1);
                    playerInventory->AddItemOrDelete(pPortal2);

                    playerInventory->AddItemOrDelete(pBoots);
                    playerInventory->AddItemOrDelete(pDentedHelmet);
                    playerInventory->AddItemOrDelete(pRustyChestplate);
                    playerInventory->AddItemOrDelete(pCopperVambraces);
                    playerInventory->AddItemOrDelete(pKilt);
                    playerInventory->AddItemOrDelete(pTheezys);
                    playerInventory->AddItemOrDelete(pFauxLeatherGloves);
                    playerInventory->AddItemOrDelete(pLapisLazuliRing);

                    playerInventory->AddItemOrDelete(pBow);
                    playerInventory->AddItemOrDelete(pSpear);
                    playerInventory->AddItemOrDelete(pHealHeart);
                    playerInventory->AddItemOrDelete(pHurtHeart);
                    playerInventory->AddItemOrDelete(pBurnHeart);
                }

                if (wolf::Input::IsKeyJustDown(GLFW_KEY_2)) {
                    playerInventory->AddGold(10);
                }

                if (wolf::Input::IsKeyJustDown(GLFW_KEY_3)) {
                    playerInventory->TakeGold(5);
                }
            }

            playerInventory->ShowToggleButtonGUI();
            playerInventory->ShowInventoryGUI();
        }

        // DEBUG: Noclip hotkey
        if (m_debugHotkeys && wolf::Input::IsKeyJustDown(GLFW_KEY_SLASH))
        {
            auto* pCollider = m_pPlayerObject->GetComponent<ColliderComponent>();
            if (pCollider)
            {
                m_noClip = !m_noClip;
                if (m_noClip)
                {
                    pCollider->SetActive(false);
                    pCollider->SetColliderType(ColliderComponent::ColliderType::HITBOX);
                }
                else
                {
                    pCollider->SetActive(true);
                    pCollider->SetColliderType(ColliderComponent::ColliderType::HITHURTBOXDR);
                }
            }
        }

        // Update all player controllers
        for (auto&&[_, controller] : m_pGameInstance->GetScene().Each<PlayerController>())
        {
            controller.Update(delta);
        }

        // Update boss controller
        for (auto&&[_, controller] : m_pGameInstance->GetScene().Each<BossController>())
        {
            controller.Update(delta);
        }


        // Update all minitaur controllers
        // First pass: Update all minitaur controllers (without deletion)
        for (auto&& [_, minitaurController] : m_pGameInstance->GetScene().Each<MinitaurController>())
        {
            minitaurController.Update(delta);  // Update logic for Minitaurs
        }
        for (auto&& [_, harpyController] : m_pGameInstance->GetScene().Each<HarpyController>())
        {
            harpyController.Update(delta);  // Update logic for Harpies
        }
        for (auto&& [_, gorgonController] : m_pGameInstance->GetScene().Each<GorgonController>())
        {
            gorgonController.Update(delta);  // Update logic for Harpies
        }
        for (auto&& [_, trigger] : m_pGameInstance->GetScene().Each<TriggerComponent>()) {
            trigger.Update(delta);
        }
        for (auto&& [_, trap] : m_pGameInstance->GetScene().Each<TrapComponent>()) {
            trap.Update(delta);
        }

        for (auto&& [_, boulder] : m_pGameInstance->GetScene().Each<BoulderTrapComponent>()) {
            boulder.Update(delta);
        }
            
        for (auto&& [_, throwable] : m_pGameInstance->GetScene().Each<ThrowableObjectComponent>()) 
        {
            throwable.Update(delta);  // Update logic for throwable objects
        }
        
        // Update all animated sprites
        for (auto&&[_, anim] : m_pGameInstance->GetScene().Each<AnimatedSprite2D>())
        {
            anim.Update(delta);
        }

        for (auto&&[_, homing] : m_pGameInstance->GetScene().Each<HomingComponent>())
        {
            homing.Update(delta);
        }

        for(auto&& [_, attackDamageComponent] : m_pGameInstance->GetScene().Each<AttackDamageComponent>())
        {
            attackDamageComponent.Update(delta);
        }

        // Update all dropped items
        for (auto&&[_, itemDrop] : m_pGameInstance->GetScene().Each<DroppedItemComponent>())
        {
            itemDrop.Update(delta);
        }

        // Update the NPCs
        for (auto&&[_, npc] : m_pGameInstance->GetScene().Each<NPCComponent>()) {
            npc.Update(delta);
        }

        // Inflict status effects upon the player
        for (auto&& [_, status] : m_pGameInstance->GetScene().Each<StatusComponent>())
        {
            status.Update(delta);
        }

        // Display all open chest GUIs
        const auto& playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        for (auto&&[_, chestInventory, transform, sprite] : m_pGameInstance->GetScene().Each<ChestInventoryComponent, wolf::Transform2D, AnimatedSprite2D>())
        {
            // Show GUI
            chestInventory.ShowInventoryGUI();

            // Distance checking
            if (glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f)
            {
                // Player is in range of the chest, display tooltip
                std::string tooltip = chestInventory.IsOpen() ? "Press E to Close Chest" : "Press E to Open Chest";
                ShowTooltip(tooltip);

                if (wolf::Input::IsKeyJustDown(GLFW_KEY_E))
                {
                    auto name = sprite.GetCurrentAnimation()->m_strName;
                    if (chestInventory.IsOpen()) {
                        wolf::Audio::Play("data/sounds/sfx_chest_close.wav", 0.8f);
                        sprite.SetAnimation(name.find("Open") != std::string::npos ? name.replace(name.find("Open"), 4, "Closed") : name);
                    }
                    else {
                        wolf::Audio::Play("data/sounds/sfx_chest_open.wav", 0.8f);
                        sprite.SetAnimation(name.find("Closed") != std::string::npos ? name.replace(name.find("Closed"), 6, "Open") : name);
                    }
                    
                    chestInventory.ToggleOpen();
                    
                    if (!chestInventory.IsOpen()) m_pPlayerObject->GetComponent<PlayerInventoryComponent>()->Close();
                    break;
                }
            }
            else
            {
                // Close chest if the player walks away
                if (chestInventory.IsOpen())
                {
                    chestInventory.Close();
                    wolf::Audio::Play("data/sounds/sfx_chest_close.wav", 0.8f);
                    auto name = sprite.GetCurrentAnimation()->m_strName;
                    sprite.SetAnimation(name.find("Open") != std::string::npos ? name.replace(name.find("Open"), 4, "Closed") : name);
                    m_pPlayerObject->GetComponent<PlayerInventoryComponent>()->Close();
                }
            }
        }

        // Trapped chests
        for (auto&&[_, trappedChest, transform, sprite] : m_pGameInstance->GetScene().Each<TrappedChestComponent, wolf::Transform2D, AnimatedSprite2D>())
        {
            // Update trapped chests
            trappedChest.Update(delta);
            // Distance checking
            if (glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f)
            {
                if(trappedChest.IsOpen() == false)
                {
                    std::string tooltip = "Press E to Open Chest";
                    ShowTooltip(tooltip);
                    if (wolf::Input::IsKeyJustDown(GLFW_KEY_E))
                    {
                        trappedChest.OpenTrappedChest();
                    }
                }
            }
        }

        // Display all open dispensary GUIs
        for (auto&&[_, dispensaryInventory, transform] : m_pGameInstance->GetScene().Each<DispensaryInventoryComponent, wolf::Transform2D>())
        {

            // If the dispensary has an animated sprite we're going to want to retrieve it
            AnimatedSprite2D* dispensarySprite = dispensaryInventory.GetGameObject()->GetComponent<AnimatedSprite2D>();

            // Show GUI
            dispensaryInventory.ShowInventoryGUI();

            // Distance checking
            if (glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f)
            {
                // Player is in range of the chest, display tooltip
                std::string tooltip = dispensaryInventory.IsOpen() ? "Press E to Close Daedalus Dispensary" : "Press E to Open Daedalus Dispensary";
                ShowTooltip(tooltip);

                // When you interact with the dispensary
                if (wolf::Input::IsKeyJustDown(GLFW_KEY_E))
                {
                    // Either open or close it
                    dispensaryInventory.ToggleOpen();

                    // If the dispensary has an AnimatedSprite
                    if (dispensarySprite) {
                        // Play the activation animation when we open it
                        if (dispensaryInventory.IsOpen()) {
                            dispensarySprite->SetAnimation("Activate");
                        }
                        else {
                            // And set it back to inactive when we close it
                            dispensarySprite->SetAnimation("Deactivate");
                        }
                    }

                    // Hide the child icon
                    for (auto& child : dispensaryInventory.GetGameObject()->GetChildren()) {
                        AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                        if (anim) {
                            anim->SetAnimation("Transparent");
                        }
                    }

                    // Also, if we close it, close the player inventory as well
                    if (!dispensaryInventory.IsOpen()) m_pPlayerObject->GetComponent<PlayerInventoryComponent>()->Close();
                    break;
                }
            }
            else
            {
                // Close dispensary if the player walks away
                if (dispensaryInventory.IsOpen())
                {
                    dispensaryInventory.Close();

                    // If the dispensary has an AnimatedSprite, play the inactive animation
                    if (dispensarySprite) {
                        dispensarySprite->SetAnimation("Deactivate");
                        wolf::EventManager::TriggerEvent(LightToggleEvent(dispensaryInventory.GetGameObject()->GetID(), false));
                    }

                    // Hide the child icon
                    for (auto& child : dispensaryInventory.GetGameObject()->GetChildren()) {
                        AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                        if (anim) {
                            anim->SetAnimation("Transparent");
                        }
                    }

                    // Close the player's inventory as well
                    m_pPlayerObject->GetComponent<PlayerInventoryComponent>()->Close();
                }
            }
        }
        
        for (auto&&[_, droppedItem, transform] : m_pGameInstance->GetScene().Each<DroppedItemComponent, wolf::Transform2D>())
        {
            // Distance checking
            if (glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f)
            {
                // Player is in range of the chest, display tooltip
                std::string tooltip = "Press E to pickup";
                ShowTooltip(tooltip);

                if (wolf::Input::IsKeyJustDown(GLFW_KEY_E))
                {
                    droppedItem.PickUpItem();
                    break;
                }
            }
        }

        for (auto&&[_, npc, transform] : m_pGameInstance->GetScene().Each<NPCComponent, wolf::Transform2D>()) {
            // Distance check
            if (glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f) {
                // Player is in range of the NPC so we display the tooltip
                std::string tooltip = "Press E to talk to " + npc.GetName();
                ShowTooltip(tooltip);

                // And if the player interacts with the NPC we play their next dialogue/cutscene
                if (wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
                    npc.PlayNextDialogue();
                    break;
                }
            }
        }

        // Display all open merchant GUIs
        for (auto&&[_, merchantInventory, transform, sprite] : m_pGameInstance->GetScene().Each<MerchantInventoryComponent, wolf::Transform2D, AnimatedSprite2D>())
        {
            // Show GUI
            merchantInventory.ShowInventoryGUI();

            // If the player walks too far away
            if (!(glm::distance(transform.GetGlobalPosition(), playerPos) < 128.0f))
            {
                // And the merchant GUI is open
                if (merchantInventory.IsOpen()) {
                    // Close it (and the player's inventory)
                    merchantInventory.Close();
                    m_pPlayerObject->GetComponent<PlayerInventoryComponent>()->Close();
                }
            }
        }

        // Trigger CutsceneDialogueEvent when pressing 9
        if (m_debugHotkeys && wolf::Input::IsKeyJustDown(GLFW_KEY_9))
        {
            // Trigger both cutscene and dialogue with IDs
            wolf::EventManager::TriggerEvent(DialogueAndCutsceneEvent("intro_sequence", "data/DialogueAndCutscenes.yaml"));
        }

        this->m_pPathfindingManager->UpdateEntities(delta);

        // Update velocity components to apply friction and decelerate objects
        for (auto&& [_, velocity] : m_pGameInstance->GetScene().Each<VelocityComponent>()) {
            velocity.Update(delta);  // Update velocity with friction and other forces
        }


        // Update collisions
        this->m_pColliderManager->Update(delta);
        // Apply velocity for all objects with Transform2D and VelocityComponent
        for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>()) {
            if (!velocity.IsActive()) continue;
            transform.Translate(velocity.GetVelocity() * delta);
        }
        ConvertPlayerTileToGold();

        auto playerPosition = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::ivec2 currentChunk = m_pLabyrinthManager->GetChunkID(playerPosition);

        if (m_visitedChunks.find(currentChunk) == m_visitedChunks.end()) {
            m_visitedChunks.insert(currentChunk);
        }

        // Toggle expanded map view
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_M)) {
            m_isMapExpanded = !m_isMapExpanded;
        }
        
        // Base update for all game objects and components in the scene
        m_pGameInstance->GetScene().Update(delta);

        // Update the lights in the scene
        for (auto&& [_, LightComponent] : m_pGameInstance->GetScene().Each<LightComponent>()) {
            LightComponent.Update(delta);
        }

        // Update damage indicators
        for (auto&& [_, health] : m_pGameInstance->GetScene().Each<HealthComponent>()) {
            health.UpdateDamageIndicators(delta);
        }

        for (auto&& [_, monsterSpawner] : m_pGameInstance->GetScene().Each<MonsterSpawnerComponent>()) {
            monsterSpawner.Update(delta);
        }

        m_particleSystem->Update(delta);
        // Toggle particle system editor with Right Alt
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT_ALT) && m_debugHotkeys) {
            m_particleSystem->ToggleEditor();
        }
        
        // Call the editor function inside update
        m_particleSystem->ShowEditor();

        if (m_pNavMeshComponent)
        {
            static int updateCounter = 0;
            const int updateFrequency = 10;
            
            updateCounter++;
            if (updateCounter % updateFrequency == 0) // update every 10 frames
            {
                m_navMeshObstacles.clear();
                m_navMeshObstacles.push_back(m_pPlayerObject);

                for (auto&& [_, controller] : m_pGameInstance->GetScene().Each<MinitaurController>())
                m_navMeshObstacles.push_back(controller.GetGameObject());

                for (auto&& [_, controller] : m_pGameInstance->GetScene().Each<GorgonController>())
                m_navMeshObstacles.push_back(controller.GetGameObject());
                
                for (auto&& [_, component] : m_pGameInstance->GetScene().Each<NPCComponent>())
                m_navMeshObstacles.push_back(component.GetGameObject());

                m_pNavMeshComponent->UpdateDynamicObstacles(m_navMeshObstacles);
            }
        }
        
    }

    // Dispatch events
    wolf::EventManager::Dispatch();
}

void PlayState::Render(float delta)
{
    // All of the background render code is done here anyway, might as well not duplicate it
    BackgroundRender(delta);

    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController)
        playerController->Render(delta);
    
    // Render damage indicators
    for (auto&& [_, health] : m_pGameInstance->GetScene().Each<HealthComponent>())
    {
        health.RenderDamageIndicators();
    }

    // Render status effect icons
    for (auto&& [_, playerController, status] : m_pGameInstance->GetScene().Each<PlayerController, StatusComponent>())
    {
        status.RenderPlayerSEIcons();
    }

    RenderMap();

    if (m_particleSystem) {
        m_particleSystem->Render();
    }

    GLShapesRenderer::GetInstance()->RenderAndDeleteLines();
    GLShapesRenderer::GetInstance()->RenderAndDeleteTriangles();

}


void PlayState::BackgroundUpdate(float delta)
{
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
    
    // Update the lights in the scene
    for (auto&& [_, LightComponent] : m_pGameInstance->GetScene().Each<LightComponent>()) {
        LightComponent.Update(delta);
    }
}

void PlayState::BackgroundRender(float delta)
{
    LightComponent::ClearFBO();

    wolf::Scene* scene = &m_pGameInstance->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    if(camera != nullptr)
    {
        glm::vec2 viewSize = camera->GetViewSize();
        m_pFBO->SetTexSize(viewSize.x, viewSize.y);
        m_pFBO->SetWindowSize(viewSize.x, viewSize.y);
    }

    // Bind framebuffer for rendering scene - leave out UI elements
    m_pFBO->Bind();

    // Render the game's scene
    m_pGameInstance->GetScene().Render(delta);

    // Bind the lighting FBO
    LightComponent::BindFBOAndBlendFunc();

    // Render light geometry to the lights' shared FBO
    for (auto&& [_, lightComp] : m_pGameInstance->GetScene().Each<LightComponent>()) {
        lightComp.RenderLightToFBO();
    }

    // Unbind the lighting FBO
    LightComponent::UnbindFBOAndBlendFunc();

    // Blend the light FBO with the screen
    LightComponent::BlendFBOAndScreen();

    // Render fluid systems
    for (auto&&[_, system] : m_pGameInstance->GetScene().Each<BoundedFluidSystem2D>())
    {
        if (system.IsIgnoreLighting()) system.Render(delta);
    }

    // Build map of animated sprites to render by layer
    std::map<int, std::vector<std::pair<AnimatedSprite2D*, wolf::Transform2D*>>> sortedAnimatedSprites;
    for (auto&&[_, sprite, transform] : m_pGameInstance->GetScene().Each<AnimatedSprite2D, wolf::Transform2D>())
    {
        // Ignore sprites that were rendered during the lighting (scene) pass
        if (sprite.IsLightingEnabled()) continue;
        
        int layer = sprite.GetLayer();

        // Add new spritebatch if it doesn't exist
        if (!sortedAnimatedSprites.contains(layer)) sortedAnimatedSprites[layer] = {};

        // Push back the next sprite
        sortedAnimatedSprites[layer].push_back(std::make_pair<AnimatedSprite2D*, wolf::Transform2D*>(&sprite, &transform));
    }

    // Render all animated sprites in order
    for (auto iter = sortedAnimatedSprites.begin(); iter != sortedAnimatedSprites.end(); ++iter)
    {
        auto& batch = iter->second;
        for (auto& pair : batch)
        {
            pair.first->Draw(pair.second->GetGlobalPosition(), pair.second->GetGlobalRotation(), pair.second->GetGlobalScale());
        }
    }

    if (m_pNavMeshComponent && m_pNavMeshComponent->IsDebugDrawEnabled())
    {
        m_pNavMeshComponent->DebugDraw();
    }
    
    // Bind to default framebuffer(screen)
    wolf::FrameBuffer::BindDefault();

    // Query postprocessing effects based on current active status effects of player
    std::vector<Postprocessor::Effect> effects;
    for (auto&& [_, playerController, status] : m_pGameInstance->GetScene().Each<PlayerController, StatusComponent>())
    {
        if(status.IsStatusEffectActive(StatusComponent::StatusEffectType::BURNING))
        {
            effects.push_back(Postprocessor::Effect::BURNING);
        }
        
        if(status.IsStatusEffectActive(StatusComponent::StatusEffectType::POISONED))
        {
            effects.push_back(Postprocessor::Effect::POISONED);
        }

        if(status.IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
        {
            effects.push_back(Postprocessor::Effect::GRAYSCALE);
        }
        break;
    }

    // Apply heat distortion if there are active fire tiles
    if(TileFireManager::GetInstance()->GetBurningFireTilesCount() > 0)
    {    
        effects.push_back(Postprocessor::Effect::HEAT_DISTORTION);
    }
    
    // If there are one or more effects, pass framebuffer texture & effects to Postprocessor to postprocess
    if(effects.size() > 0)
    {
        Postprocessor::GetInstance()->Postprocess(m_pFBO->GetTextureID(), effects);
    }
    // If not, copy texture to screen
    else
    {
        m_pFBO->Blit();
    }
}

void PlayState::CreatePlayer()
{
    // Create player object with transform
    m_pPlayerObject = &m_pGameInstance->GetScene().CreateObject2D();
    m_pGameInstance->GetScene().SetPlayerID(m_pPlayerObject->GetID());
    
    // Register the player (Theseus) in the shared context
    m_pGameInstance->GetSharedContext().RegisterEntity("Theseus", m_pPlayerObject->GetID());

    // Add velocity
    m_pPlayerObject->AddComponent<VelocityComponent>();

    // Add inventory
    auto& inventory = m_pPlayerObject->AddComponent<PlayerInventoryComponent>(16, 4, ImVec2(50, 50));

    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(7.0f, 10.0f), glm::vec2(-4.0f, -3.0f));

    // Add health
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(800);

    // Add status component and status effect
    auto& status = m_pPlayerObject->AddComponent<StatusComponent>();

    // Add player controller and initialize 
    // NOTE: This manages all player animations and the animated sprite component for the player
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();
    playerController.LateInitialize();

    // Add ParticleComponent to the player
    auto& playerParticles = m_pPlayerObject->AddComponent<ParticleComponent>();
    // Register the player's ParticleComponent in the ParticleSystem
    m_particleSystem->RegisterComponent(&playerParticles);
    // Start player at the labyrinth spawn location and scale appropriately
    auto& transform = *m_pPlayerObject->GetComponent<wolf::Transform2D>();
    transform.SetScale(glm::vec2(3));
}

void PlayState::OnDialogueAndCutsceneTriggered(const DialogueAndCutsceneEvent& event) {
    // std::cout << "Triggered sequence: " << event.sequenceID << std::endl;

    // Push the DialogueAndCutsceneState onto the game state stack
    auto* dialogueAndCutsceneState = new DialogueAndCutsceneState(m_pStateManager, m_pGameInstance, event.dialogueFilePath, event.triggerNPCID);
    dialogueAndCutsceneState->LoadSequence(event.sequenceID);  // Start the specific sequence
    m_pStateManager->PushState(dialogueAndCutsceneState);
}



// Event handler to spawn traps when a trigger is triggered
void PlayState::OnTriggerEvent(const TriggerEvent& event) {
    auto* triggerObject = event.m_pTriggerObject;
    if (!triggerObject) return;

    auto* transform = triggerObject->GetComponent<wolf::Transform2D>();
    if (!transform) {
        wolf::Error("Trigger object has no Transform2D component!");
        return;
    }
    glm::vec2 triggerPosition = transform->GetGlobalPosition();
    TriggerPurpose purpose = event.m_purpose;

    switch (purpose) {
        case TriggerPurpose::SPIKE_TRAP: {

            // Play sound effect
            wolf::Audio::Play("data/sounds/sfx_spike_trap.wav", 0.9f);

            auto& trapObj = m_pGameInstance->GetScene().CreateObject2D();
            auto& trapSprite = trapObj.AddComponent<wolf::Sprite2D>("data/textures/SpikesExtended.png");
            trapSprite.SetOriginToCenterOfTexture();
            trapSprite.SetLayer(1);
            auto* trapTransform = trapObj.GetComponent<wolf::Transform2D>();
            if (!trapTransform) {
                trapTransform = &trapObj.AddComponent<wolf::Transform2D>();
            }
            trapTransform->SetPosition(triggerPosition);
            trapTransform->SetScale(glm::vec2(3.0f));

            auto& velocity = trapObj.AddComponent<VelocityComponent>();
            velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

            // Add collider for the trap
            auto& trapCollider = trapObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
            trapCollider.AddColliderBox(glm::vec2(24.0f, 24.0f), glm::vec2(-12.0f, 12.0f));
            // Add TrapComponent with some parameters (e.g., 50 damage, 5 seconds lifespan)
            trapObj.AddComponent<TrapComponent>(triggerObject->GetComponent<TriggerComponent>(), 50.0f, 1.0f, m_pColliderManager, 0.0f);
            // wolf::Log("Spike trap triggered!");
            break;
        }
        case TriggerPurpose::BOULDER_TRAP: {
            // Retrieve the room data for the trigger's position
            auto tilePosition = m_pLabyrinthManager->GetTilePosition(triggerPosition);
            auto roomDataOpt = m_pLabyrinthManager->GetRoom(tilePosition);
        
            if (!roomDataOpt.has_value()) {
                // wolf::Log("Boulder trap triggered, but no valid room found!");
                break;
            }
        
            const auto& roomData = roomDataOpt.value();
            const auto& roomBounds = roomData.m_bounds; // IRectangle struct
        
            // Create the boulder object
            auto& boulderObj = m_pGameInstance->GetScene().CreateObject2D();
        
            // Set up sprite
            auto& boulderSprite = boulderObj.AddComponent<wolf::Sprite2D>("data/textures/Boulder.png");
            boulderSprite.SetOriginToCenterOfTexture();
            boulderSprite.SetLayer(1);
        
            // Ensure the transform component exists
            auto* boulderTransform = boulderObj.GetComponent<wolf::Transform2D>();
            if (!boulderTransform) {
                boulderTransform = &boulderObj.AddComponent<wolf::Transform2D>();
            }
        
            // Get the player's position
            wolf::GameObject* player = m_pLabyrinthManager->GetPlayer();
            glm::vec2 playerPosition = player ? player->GetComponent<wolf::Transform2D>()->GetGlobalPosition() : triggerPosition;
        
            // Possible directions mapped to available space (how far it can go before hitting a wall)
            std::vector<std::pair<BoulderDirection, int>> directionOptions;
        
            // Helper lambda to check how far the boulder can go in a direction
            auto getDistance = [&](int dx, int dy) -> int {
                glm::ivec2 checkPos = tilePosition;
                int distance = 0;
        
                while (true) {
                    checkPos += glm::ivec2(dx, dy);
                    if (!m_pLabyrinthManager->GetRoom(checkPos).has_value()) {
                        break; // Stop if we leave the room
                    }
                    distance++;
                }
                return distance;
            };
        
            // Check each possible direction and measure its available distance
            int upDist = getDistance(0, 1);
            int downDist = getDistance(0, -1);
            int leftDist = getDistance(-1, 0);
            int rightDist = getDistance(1, 0);
        
            if (upDist > 0) directionOptions.emplace_back(BoulderDirection::UP, upDist);
            if (downDist > 0) directionOptions.emplace_back(BoulderDirection::DOWN, downDist);
            if (leftDist > 0) directionOptions.emplace_back(BoulderDirection::LEFT, leftDist);
            if (rightDist > 0) directionOptions.emplace_back(BoulderDirection::RIGHT, rightDist);
        
            if (directionOptions.empty()) {
                // wolf::Log("No valid directions found for boulder trap!");
                break;
            }
        
            BoulderDirection chosenDirection;
        
            // **90% chance to go towards player, 10% to use weighted random**
            if (m_pLabyrinthManager->m_rng.NextInt(0, 9) < 9 && player) { // 90% probability
                glm::vec2 directionToPlayer = glm::normalize(playerPosition - triggerPosition);
                BoulderDirection bestDirection = BoulderDirection::UP;
                float bestAlignment = -1.0f;  // Tracks best alignment score

                // Find the direction most aligned with the player
                for (const auto& [dir, dist] : directionOptions) {
                    glm::vec2 dirVec;
                    switch (dir) {
                        case BoulderDirection::UP: dirVec = {0.0f, 1.0f}; break;
                        case BoulderDirection::DOWN: dirVec = {0.0f, -1.0f}; break;
                        case BoulderDirection::LEFT: dirVec = {-1.0f, 0.0f}; break;
                        case BoulderDirection::RIGHT: dirVec = {1.0f, 0.0f}; break;
                    }

                    float alignment = glm::dot(directionToPlayer, dirVec);
                    if (alignment > bestAlignment) {
                        bestAlignment = alignment;
                        bestDirection = dir;
                    }
                }
                chosenDirection = bestDirection;
            } else {
                // Weighted random selection favoring longer paths
                std::vector<BoulderDirection> weightedDirections;
                for (const auto& [dir, dist] : directionOptions) {
                    for (int i = 0; i < dist; ++i) { // More distance = higher chance of selection
                        weightedDirections.push_back(dir);
                    }
                }
                chosenDirection = weightedDirections[m_pLabyrinthManager->m_rng.NextInt(0, static_cast<int>(weightedDirections.size()) - 1)];
            }
                    
            // Assign velocity based on chosen direction
            constexpr float boulderSpeed = 100.0f;
            glm::vec2 velocity(0.0f);
        
            switch (chosenDirection) {
                case BoulderDirection::UP:
                    velocity = {0.0f, boulderSpeed};
                    break;
                case BoulderDirection::DOWN:
                    velocity = {0.0f, -boulderSpeed};
                    break;
                case BoulderDirection::LEFT:
                    velocity = {-boulderSpeed, 0.0f};
                    break;
                case BoulderDirection::RIGHT:
                    velocity = {boulderSpeed, 0.0f};
                    break;
            }
        
            // **Smart Offset: Place it one tile away in the opposite direction**
            constexpr float tileSize = 46.0f;
            glm::vec2 spawnPosition = triggerPosition;
        
            // Convert room bounds to vec2 to ensure proper calculations
            glm::vec2 roomOrigin = glm::vec2(roomBounds.m_origin);
            glm::vec2 roomSize = glm::vec2(roomBounds.m_size);

            // **Offset in the opposite direction of movement**
            switch (chosenDirection) {
                case BoulderDirection::UP:
                    spawnPosition.y -= tileSize;
                    break;
                case BoulderDirection::DOWN:
                    spawnPosition.y += tileSize;
                    break;
                case BoulderDirection::LEFT:
                    spawnPosition.x += tileSize;
                    break;
                case BoulderDirection::RIGHT:
                    spawnPosition.x -= tileSize;
                    break;
            }

            glm::vec2 roomMin = glm::vec2(roomBounds.m_origin);
            glm::vec2 roomMax = glm::vec2(roomBounds.m_origin) + glm::vec2(roomBounds.m_size) - glm::vec2(tileSize);

            // **Check if the spawn position is outside the room bounds**
            if (spawnPosition.x < roomMin.x || spawnPosition.x > roomMax.x ||
                spawnPosition.y < roomMin.y || spawnPosition.y > roomMax.y) {
                
                spawnPosition = triggerPosition; // If out of bounds, spawn at the trigger position
            }

            // **Set the adjusted initial position**
            boulderTransform->SetPosition(spawnPosition);
            boulderTransform->SetScale(glm::vec2(3.0f)); // Scale the boulder
        
            // Add Velocity Component for movement
            auto& velocityComponent = boulderObj.AddComponent<VelocityComponent>();
            velocityComponent.SetVelocity(velocity);
        
            // Add Collider
            auto& boulderCollider = boulderObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDR, 0, 1);
            boulderCollider.AddColliderBox(glm::vec2(28.0f, 28.0f), glm::vec2(-14.0f, 14.0f));
        
            // Add BoulderTrapComponent with the chosen direction
            boulderObj.AddComponent<BoulderTrapComponent>(triggerObject->GetComponent<TriggerComponent>(), m_pColliderManager, chosenDirection, boulderSpeed, 5.0f);
        
            // wolf::Log("Boulder trap triggered, rolling in direction: " + std::to_string(static_cast<int>(chosenDirection)));
            break;
        }         

        case TriggerPurpose::BOSS: {
            
            TileFireManager::GetInstance()->SetPropagationActiveness(false);

            // Stop the background music
            wolf::Audio::Stop("data/sounds/bgm_maze.wav");
            
            // Begin the bossfight
            wolf::Log("BOSSFIGHT STARTED");
            m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetPosition(m_bossfightPlayerPos);
            m_pBoss->GetComponent<BossController>()->StartBossfight();

            // Zoom out camera
            m_bossZoomTimer.Restart();

            // Change filter mode on all tilemaps
            for (auto&&[_, tilemap] : m_pGameInstance->GetScene().Each<wolf::TileMap>())
            {
                tilemap.SetFilterMode(GL_LINEAR_MIPMAP_LINEAR);
            }

            // Set door areas to wall tiles
            for (const auto& door : m_bossRoomDoorTiles)
            {
                m_pLabyrinthManager->SetTile(door.x, door.y, Tile::WallMinotaur);
            }

            // Create a collider that covers all walls of the boss room
            auto& object = m_pBoss->GetScene().CreateObject2D();
            m_pBossWalls->AddChild(object);

            // Set scale
            auto& transform = *object.GetComponent<wolf::Transform2D>();
            transform.SetScale(glm::vec2(3.0f));

            // Add colliders for boss room walls
            auto& collider = object.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, false, false);
            collider.AddColliderBox(glm::vec2((m_bossRoomSize.x + 2) * 96, 96), m_pLabyrinthManager->GetWorldPosition(m_bossRoomOrigin) + glm::vec2(-96.0f, 0.0f));
            collider.AddColliderBox(glm::vec2((m_bossRoomSize.x + 2) * 96, 96), m_pLabyrinthManager->GetWorldPosition(m_bossRoomOrigin) + glm::vec2(-96, (m_bossRoomSize.y + 1) * 96));
            collider.AddColliderBox(glm::vec2(96, (m_bossRoomSize.y + 2) * 96), m_pLabyrinthManager->GetWorldPosition(m_bossRoomOrigin) + glm::vec2(-96, (m_bossRoomSize.y + 1) * 96));
            collider.AddColliderBox(glm::vec2(96, (m_bossRoomSize.y + 2) * 96), m_pLabyrinthManager->GetWorldPosition(m_bossRoomOrigin) + glm::vec2((m_bossRoomSize.x + 1) * 96 - 96, (m_bossRoomSize.y + 1) * 96));
            
            // Deactivate all chunks
            m_pLabyrinthManager->StartBossfight();
            
            break;
        }

        case TriggerPurpose::POISON_TRAP:
        {
            // Retrieve the room data for the trigger's position
            auto tilePosition = m_pLabyrinthManager->GetTilePosition(triggerPosition);
            auto roomDataOpt = m_pLabyrinthManager->GetRoom(tilePosition);
            if (!roomDataOpt.has_value()) {
                wolf::Log("No valid room found for poison trap!");
                break;
            }

            // Grab room data
            const auto& room = roomDataOpt.value();
            const auto& bounds = room.m_bounds;

            // Calculate simulation bounds
            auto simBounds = wolf::Rectangle(bounds);
            simBounds.m_top *= 96;
            simBounds.m_left *= 96;
            simBounds.m_right *= 96;
            simBounds.m_bottom *= 96;

            // Create the fluid system
            auto& fluidObj = m_pGameInstance->GetScene().CreateObject2D();
            auto& fluidSystem = fluidObj.AddComponent<BoundedFluidSystem2D>(simBounds);

            // Edit poison colors
            fluidSystem.SetFluidColor(glm::vec4(0.01f, 0.5f, 0.25f, 0.75f));
            fluidSystem.SetWaveColor(glm::vec4(0.24f, 0.7f, 0.36f, 0.75f));
            fluidSystem.SetCausticColor(glm::vec4(0.32f, 0.78f, 0.26f, 0.75f));
            fluidSystem.SetCausticFrequency(0.4f);

            glm::vec2 center = glm::vec2(simBounds.m_left + simBounds.GetWidth() / 2, simBounds.m_bottom + simBounds.GetHeight() / 2);

            // Add a timed spout to spawn poison!
            fluidSystem.AddTimedSpout(center, 2.0f, room.m_bounds.m_size.x * room.m_bounds.m_size.y * 8);

            // Add a delayed drain to remove all the fluid after
            fluidSystem.AddTimedDrain(wolf::Circle(center, 8.0f), 20.0f, 512.0f, 250.0f, 8.0f);

            // Make sure the fluid system object gets completely destroyed after the draining is done
            fluidObj.AddComponent<TimedDestroyerComponent>(20);

            // Add the fluid object as a child of the trigger
            event.m_pTriggerObject->AddChild(fluidObj);

            // Reactivate the trigger after all is finished (20 seconds)
            event.m_pTriggerObject->GetComponent<TriggerComponent>()->ReactivateDelayed(20);

            wolf::Audio::Play("data/sounds/sfx_liquid_flow.wav", 0.5f);
            wolf::Audio::Play("data/sounds/sfx_liquid_bubbling.wav", 0.4f);

            break;
        }

        case TriggerPurpose::LAVA_TRAP:
        {
            // Retrieve the room data for the trigger's position
            auto tilePosition = m_pLabyrinthManager->GetTilePosition(triggerPosition);
            auto roomDataOpt = m_pLabyrinthManager->GetRoom(tilePosition);
            if (!roomDataOpt.has_value()) {
                wolf::Log("No valid room found for lava trap!");
                break;
            }

            // Grab room data
            const auto& room = roomDataOpt.value();
            const auto& bounds = room.m_bounds;

            // Calculate simulation bounds
            auto simBounds = wolf::Rectangle(bounds);
            simBounds.m_top *= 96;
            simBounds.m_left *= 96;
            simBounds.m_right *= 96;
            simBounds.m_bottom *= 96;

            // Create the fluid system
            auto& fluidObj = m_pGameInstance->GetScene().CreateObject2D();
            auto& fluidSystem = fluidObj.AddComponent<BoundedFluidSystem2D>(simBounds);

            // Edit lava colors
            fluidSystem.SetFluidColor(glm::vec4(1.0f, 0.353, 0.0f, 0.918f));
            fluidSystem.SetWaveColor(glm::vec4(1.0f, 0.453, 0.0f, 0.918f));
            fluidSystem.SetCausticColor(glm::vec4(1.0f, 0.553, 0.0f, 0.918f));
            fluidSystem.SetCausticFrequency(0.5f);
            fluidSystem.SetIgnoreLighting(true);

            glm::vec2 center = glm::vec2(simBounds.m_left + simBounds.GetWidth() / 2, simBounds.m_bottom + simBounds.GetHeight() / 2);

            // Add a timed spout to spawn lava!
            fluidSystem.AddTimedSpout(center, 2.0f, room.m_bounds.m_size.x * room.m_bounds.m_size.y * 8);

            // Add a delayed drain to remove all the fluid after
            fluidSystem.AddTimedDrain(wolf::Circle(center, 8.0f), 20.0f, 512.0f, 250.0f, 8.0f);

            // Make sure the fluid system object gets completely destroyed after the draining is done
            fluidObj.AddComponent<TimedDestroyerComponent>(20);

            // Add the fluid object as a child of the trigger
            event.m_pTriggerObject->AddChild(fluidObj);

            // Reactivate the trigger after all is finished (20 seconds)
            event.m_pTriggerObject->GetComponent<TriggerComponent>()->ReactivateDelayed(20);

            wolf::Audio::Play("data/sounds/sfx_liquid_flow.wav", 0.5f);
            wolf::Audio::Play("data/sounds/sfx_liquid_bubbling.wav", 0.4f);

            break;
        }

        default:
            wolf::Log("Unsupported trigger purpose.");
            break;
    }
}

void PlayState::OnGameWinEvent(const GameWinEvent& event)
{
    m_gameCompletionTime.Pause();

    // Deactivate the player controller
    m_pPlayerObject->GetComponent<PlayerController>()->SetActive(false);

    // Play game win sound
    wolf::Audio::Play("data/sounds/sfx_game_win.wav", 1.0f);

    // Start camera shake effect for 2 seconds
    m_cameraShakeTimer.Start();

    // Get the ParticleComponent from Theseus
    auto* particleComponent = m_pPlayerObject->GetComponent<ParticleComponent>();
    if (particleComponent)
    {
        glm::vec2 playerPosition = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        // Emit 100 particles in random directions
        for (int i = 0; i < 100; ++i)
        {
            glm::vec2 randomVelocity = glm::diskRand(300.0f); 
            glm::vec4 randomColor = glm::vec4(
                m_pLabyrinthManager->m_rng.NextFloat(0.5f, 1.0f),  
                m_pLabyrinthManager->m_rng.NextFloat(0.5f, 1.0f),  
                m_pLabyrinthManager->m_rng.NextFloat(0.5f, 1.0f),  
                1.0f
            );

            float size = m_pLabyrinthManager->m_rng.NextFloat(2.0f, 5.0f);
            float lifetime = m_pLabyrinthManager->m_rng.NextFloat(1.0f, 2.0f);

            particleComponent->Emit(playerPosition, randomVelocity, randomColor, size, lifetime);
        }
    }

    // Begin fade to black
    m_fadeToBlackTimer.Start();

    // Schedule text and credits display
    m_completionMessageTimer.Start();  // Show message after fade-in
    m_showCreditsTimer.Start();  // Show credits after 5s of message
    m_returnToMainMenuTimer.Start();  // Return after credits (15s later)
}



int GetGoldVariant(int tileID) {
    switch (tileID) {
        case Tile::FloorSmallSquares:
            return Tile::FloorSmallSquaresGold;
        case Tile::FloorSquare:
            return Tile::FloorSquareGold;
        case Tile::FloorSpiral:
            return Tile::FloorSpiralGold;
        default:
            return -1; // No gold variant
    }
}


void PlayState::ConvertPlayerTileToGold() {
    if (!m_pLabyrinthManager || !m_pPlayerObject) {
        wolf::Log("PlayState: LabyrinthManager or PlayerObject is not set.");
        return;
    }

    auto* playerTransform = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    if (!playerTransform) return;

    // Calculate the bottom-center position of the player
    glm::vec2 playerPosition = playerTransform->GetGlobalPosition();
    glm::vec2 playerScale = playerTransform->GetGlobalScale();
    glm::vec2 bottomCenterPosition = playerPosition + glm::vec2(0.0f, -22.0f);
    glm::vec2 roundedPosition = glm::round(bottomCenterPosition);

    // Get tile position and ID
    glm::ivec2 tilePos = m_pLabyrinthManager->GetTilePosition(roundedPosition);
    int currentTileID = m_pLabyrinthManager->GetTile(tilePos.x, tilePos.y);

    // Skip if the player is in the boss room
    auto roomOpt = m_pLabyrinthManager->GetRoom(tilePos);
    if (roomOpt.has_value() && roomOpt->m_name == "Minotaur's Chamber") return;

    // Check for a gold variant
    int goldTileID = GetGoldVariant(currentTileID);
    if (goldTileID != -1) {
        m_pLabyrinthManager->SetTile(tilePos.x, tilePos.y, goldTileID);
    }
}

void PlayState::OnGameOverEvent(const GameOverEvent& event) {
    switch (event.type) {
        case GameOverType::MAIN_MENU:
            m_pStateManager->ClearAndPushState(new MainMenuState(m_pStateManager, m_pGameInstance));
            break;
        case GameOverType::EXIT:
            m_pGameInstance->Shutdown();
            break;
    }
}

void PlayState::ShowTooltip(const std::string& text)
{
    // Tooltip window code taken from Youssef's ThrowableObjectComponent
            
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

    ImGui::Begin("Tooltip###", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("%s", text.c_str());
    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

ImVec2 ToImVec2(const glm::vec2& vec) {
    return ImVec2(vec.x, vec.y);
}

glm::vec2 ToGLMVec2(const ImVec2& vec) {
    return glm::vec2(vec.x, vec.y);
}

ImU32 GetTileColor(int tileID) {
    switch (tileID) {
        // Walls
        case Tile::WallBottomLeft:
        case Tile::WallBottomRight:
        case Tile::WallBottom:
        case Tile::WallChest:
        case Tile::WallHelmet:
        case Tile::WallLeft:
        case Tile::WallMaze:
        case Tile::WallMinotaur:
        case Tile::WallPillars:
        case Tile::WallPot:
        case Tile::WallRight:
        case Tile::WallSpiral:
        case Tile::WallSquare:
        case Tile::WallTopLeft:
        case Tile::WallTopRight:
        case Tile::WallTop:
            return IM_COL32(90, 90, 90, 255); // Dark Gray for Walls

        // Floors and Bricks (Unified color with slight variations)
        case Tile::BorderedGrass:
        case Tile::Bricks:
        case Tile::FloorSmallSquares:
        case Tile::FloorSquare:
        case Tile::FloorSpiral:
            return IM_COL32(200, 200, 200, 255); // Soft Neutral Gray

        // Gold Floors (Toned down)
        case Tile::FloorSmallSquaresGold:
        case Tile::FloorSquareGold:
        case Tile::FloorSpiralGold:
            return IM_COL32(220, 185, 60, 255); // Subtle Gold

        // Grass
        case Tile::Grass:
            return IM_COL32(34, 139, 34, 255); // Forest Green for Grass

        // Default color
        default:
            return IM_COL32(128, 128, 128, 255); // Mid Gray for Default/Unknown Tiles
    }
}

void PlayState::RenderMap() {
    static float defaultZoomScale = 0.2f; // Default zoom level when not expanded
    static float expandedZoomScale = 1.0f; // Persisted zoom level for expanded map
    static bool isExpandedPrev = false; // Tracks if the map was expanded in the previous frame

    // Determine the zoom level based on whether the map is expanded
    if (m_isMapExpanded) {
        float scrollDelta = ImGui::GetIO().MouseWheel;
        expandedZoomScale = glm::clamp(expandedZoomScale + scrollDelta * 0.1f, 0.2f, 2.0f); // Adjust expanded zoom
    }

    // Update the zoom scale and reset if switching between states
    float zoomScale = m_isMapExpanded ? expandedZoomScale : defaultZoomScale;

    if (!m_isMapExpanded && isExpandedPrev) {
        defaultZoomScale = glm::clamp(expandedZoomScale * 0.5f, 0.2f, 1.0f); // Adjust default zoom to see more
    }
    isExpandedPrev = m_isMapExpanded;

    // Define map dimensions and scaling
    const float mapSize = m_isMapExpanded ? 600.0f : 200.0f; // Larger default map size for expanded view
    const float labyrinthScale = zoomScale;

    // Determine map position (top-right when small, center when expanded)
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const ImVec2 mapPosition = m_isMapExpanded
        ? ImVec2((displaySize.x - mapSize) * 0.5f, (displaySize.y - mapSize) * 0.5f) // Centered
        : ImVec2(displaySize.x - mapSize - 20.0f, 20.0f); // Top-right corner

    // Get player transform component and compute adjusted position
    const auto* playerTransform = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    assert(playerTransform != nullptr && "Player Transform2D component is missing!");
    const glm::vec2 playerPosition = playerTransform->GetGlobalPosition() + glm::vec2(-48.0f, -60.0f);

    // Precompute constants for rendering
    const float tileWorldSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    const float halfTileSizeScaled = (tileWorldSize * labyrinthScale) * 0.5f;
    const float mapCenterX = mapPosition.x + mapSize / 2.0f;
    const float mapCenterY = mapPosition.y + mapSize / 2.0f;
    const glm::ivec2 playerChunk = glm::ivec2(playerPosition / (tileWorldSize * LabyrinthManager::CHUNK_SIZE));

    // Thick stylish golden border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 4.0f); // Thicker border
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 215, 0, 255)); // Gold color
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255)); // Black background

    ImGui::SetNextWindowSize(ImVec2(mapSize, mapSize));
    ImGui::SetNextWindowPos(mapPosition);
    ImGui::Begin("ChunkMap###AlwaysVisible", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // Get window min/max for border placement
    ImVec2 windowMin = ImGui::GetWindowPos();
    ImVec2 windowMax = ImVec2(windowMin.x + mapSize, windowMin.y + mapSize);
    
    // Helper lambda for rendering tiles
    auto renderTile = [&](const glm::vec2& worldPos, ImU32 color) {
        glm::vec2 relativePos = (worldPos - playerPosition) * labyrinthScale;
        relativePos.y = -relativePos.y; // Invert Y-axis for rendering
        const ImVec2 min(mapCenterX + relativePos.x - halfTileSizeScaled, 
                         mapCenterY + relativePos.y - halfTileSizeScaled);
        const ImVec2 max(mapCenterX + relativePos.x + halfTileSizeScaled, 
                         mapCenterY + relativePos.y + halfTileSizeScaled);
        drawList->AddRectFilled(min, max, color);
        drawList->AddRect(min, max, IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);
    };

    // Render chunks and tiles
    const int chunkRenderRadius = 1; // Render surrounding chunks within 1 chunk radius
    for (int cx = -chunkRenderRadius; cx <= chunkRenderRadius; ++cx) {
        for (int cy = -chunkRenderRadius; cy <= chunkRenderRadius; ++cy) {
            glm::ivec2 chunkID = playerChunk + glm::ivec2(cx, cy);
            if (auto* chunk = m_pLabyrinthManager->GetChunk(chunkID)) {
                for (int tx = 0; tx < LabyrinthManager::CHUNK_SIZE; ++tx) {
                    for (int ty = 0; ty < LabyrinthManager::CHUNK_SIZE; ++ty) {
                        glm::ivec2 tilePos = chunkID * LabyrinthManager::CHUNK_SIZE + glm::ivec2(tx, ty);
                        int tileID = m_pLabyrinthManager->GetTile(tilePos.x, tilePos.y);
                        if (tileID > 0 && tileID != Tile::Empty) {
                            renderTile(glm::vec2(tilePos) * tileWorldSize, GetTileColor(tileID));
                        }
                    }
                }
            }
        }
    }

    // Render player position
    drawList->AddCircleFilled(ImVec2(mapCenterX, mapCenterY), 5.0f, IM_COL32(0, 255, 0, 255));
    drawList->AddCircle(ImVec2(mapCenterX, mapCenterY), 6.5f, IM_COL32(255, 255, 255, 255), 0, 1.5f);

    // Helper lambda for rendering entities
    auto renderEntity = [&](const glm::vec2& entityPos, ImU32 color) {
        glm::vec2 relativePos = ((entityPos + glm::vec2(-48.0f, -60.0f)) - playerPosition) * labyrinthScale;
        relativePos.y = -relativePos.y; // Invert Y-axis
        const ImVec2 entityMarker(mapCenterX + relativePos.x, mapCenterY + relativePos.y);
        drawList->AddCircle(entityMarker, 6.5f, IM_COL32(255, 255, 255, 255), 0, 1.5f); // Outline
        drawList->AddCircleFilled(entityMarker, 5.0f, color);                           // Marker
    };

    // Render entities by type
    for (auto&& [_, controller] : m_pGameInstance->GetScene().Each<MinitaurController>()) {
        auto* transform = controller.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(255, 0, 0, 255));
    }
    for (auto&& [_, controller] : m_pGameInstance->GetScene().Each<HarpyController>()) {
        auto* transform = controller.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(255, 255, 0, 255));
    }
    for (auto&& [_, controller] : m_pGameInstance->GetScene().Each<GorgonController>()) {
        auto* transform = controller.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(128, 0, 128, 255));
    }
    for (auto&& [_, component] : m_pGameInstance->GetScene().Each<NPCComponent>()) {
        auto* transform = component.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(0, 0, 255, 255));
    }
    for (auto&& [_, component] : m_pGameInstance->GetScene().Each<DroppedItemComponent>()) {
        auto* transform = component.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(0, 255, 255, 255));
    }
    for (auto&& [_, component] : m_pGameInstance->GetScene().Each<ChestInventoryComponent>()) {
        auto* transform = component.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(255, 165, 0, 255)); // Orange
    }
    for (auto&& [_, component] : m_pGameInstance->GetScene().Each<TrappedChestComponent>()) {
        auto* transform = component.GetGameObject()->GetComponent<wolf::Transform2D>();
        if (transform) renderEntity(transform->GetGlobalPosition(), IM_COL32(255, 165, 0, 255)); // Orange
    }


    ImGui::End();
    // --- Draw the border AFTER the minimap rendering ---
    drawList->AddRect(windowMin, windowMax, IM_COL32(255, 215, 0, 255), 8.0f, 0, 6.0f); // Thick gold border
    drawList->AddRect(windowMin, windowMax, IM_COL32(255, 165, 0, 128), 12.0f, 0, 3.0f); // Outer glow
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

wolf::GameObject& PlayState::CreateAriadneAndReturn(glm::vec2 playerPosition)
{
    // Offset position to place Ariadne on top of the player by one tile
    glm::vec2 ariadnePosition = playerPosition + glm::vec2(0.0f, LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE);

    // Specify the YAML file for Ariadne's NPC data
    std::string ariadneYamlFile = "data/ariadne_init.yaml";

    // Create Ariadne using the NPCBuilder
    wolf::GameObject& ariadne = *NPCBuilder::Instance()->BuildNPC(ariadneYamlFile);

    // Set Ariadne's position and scale
    auto& transform = *ariadne.GetComponent<wolf::Transform2D>();
    transform.SetPosition(ariadnePosition);
    transform.SetScale(glm::vec2(LabyrinthManager::SCALE));

    // Optionally register Ariadne in the shared context for reference in cutscenes
    m_pGameInstance->GetSharedContext().RegisterEntity("Ariadne", ariadne.GetID());

    // wolf::Log("Ariadne created at position: (" + std::to_string(ariadnePosition.x) + ", " + std::to_string(ariadnePosition.y) + ").");

    return ariadne;
}


void PlayState::RenderFadeOverlay(float alpha)
{
    if (alpha >= 1.0f) alpha = 1.0f;
    if (alpha <= 0.0f) return;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, alpha));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (m_showCreditsTimer.Elapsed() < 20.0f) flags |= ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin("FadeOverlay", nullptr, flags ))
    {
        if (m_showCreditsTimer.IsRunning() && m_showCreditsTimer.Elapsed() >= 20.0f)
        {
            // Style taken from PauseState
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

            ImGui::SetCursorPosY(ImGui::GetIO().DisplaySize.y * 0.6f); // Center 
            ImGui::SetCursorPosX((ImGui::GetIO().DisplaySize.x - 200.0f) * 0.5f); // Center
            if (ImGui::Button("Return to Main Menu", ImVec2(200.0f, 50.0f)))
            {
                // Return to the main menu when clicked
                wolf::EventManager::EnqueueEvent(GameOverEvent(GameOverType::MAIN_MENU));
                m_isExiting = true;
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
        }

        ImGui::End();
    }

    ImGui::PopStyleColor();
}



void PlayState::RenderTextCentered(const std::string& text, float size)
{
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.4f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // white text

    ImGui::Begin("CenteredText", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(size);
    ImGui::Text("%s", text.c_str());
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void PlayState::RenderCredits(float delta)
{
    static const char* credits[] = {
        "Theseus Development Team",
        "------------------------",
        "",
        "Project Lead: Aurora Ryder",
        "",
        "Lead Programmer: D'Anyil Landry",
        "",
        "Programmers:",
        "------------------------",
        "Aurora Ryder",
        "D'Anyil Landry",
        "Youssef Ashraf",
        "Nguyen Minh Nhat",
        "",
        "Lead Artist: Aurora Ryder",
        "",
        "Music / SFX Design: D'Anyil Landry",
        "",
        "lots of love, if you got here, ur an amazing person",
        "",
        "Thank you for playing!"
    };

    // Smooth scrolling effect
    float elapsed = m_showCreditsTimer.Elapsed();

    // Set window position and center it
    float windowWidth = 500.0f;
    float windowHeight = 420.0f;
    float baseY = ImGui::GetIO().DisplaySize.y - (elapsed * ((ImGui::GetIO().DisplaySize.y + windowHeight) / 20.0f)); // Adjust scrolling speed

    ImVec2 windowPos(ImGui::GetIO().DisplaySize.x * 0.5f - windowWidth * 0.5f, baseY);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Style adjustments
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Begin("Credits", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // Increase font size for readability
        ImGui::SetWindowFontScale(1.2f);

        // Center text
        for (const char* line : credits)
        {
            float textWidth = ImGui::CalcTextSize(line).x;
            float windowCenter = ImGui::GetWindowSize().x * 0.5f;
            ImGui::SetCursorPosX(windowCenter - textWidth * 0.5f);
            ImGui::TextUnformatted(line);
        }

        ImGui::End();
    }

    // Restore styles
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

}