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
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"
#include "DDACalculator.h"
#include "GLShapesRenderer.h"
#include "TileFireManager.h"
#include "../npcs/NPCBuilder.h"
#include "../components/NPCComponent.h"
#include <BossController.h>

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the dialogue listener
    wolf::EventManager::AddListener<DialogueAndCutsceneEvent, PlayState, &PlayState::OnDialogueAndCutsceneTriggered>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::AddListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);
 
    this->m_pColliderManager = new ColliderManager(&scene);

    // Initialize the player object
    CreatePlayer();

    // Add the main camera as a child object of the player
    auto& cameraObj = scene.CreateObject2D();
    auto& camera = cameraObj.AddComponent<wolf::Camera2D>(1280, 720);
    m_pPlayerObject->AddChild(cameraObj);
    camera.SetPosition(cameraObj.GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager and generate the default labyrinth config
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();
    m_pLabyrinthManager->m_pColliderManager = m_pColliderManager;
    m_pLabyrinthManager->LoadConfig("data/labyrinth_config.yaml");
    m_pLabyrinthManager->GenerateLabyrinth();

    GLShapesRenderer::CreateInstance();
    DDACalculator::CreateInstance(&scene);

    TileFireManager::CreateInstance(m_pLabyrinthManager);

    // Place the bossfight trigger
    const auto& rooms = m_pLabyrinthManager->GetRooms();
    for (const auto& room : rooms)
    {
        if (room.m_name != "Minotaur's Chamber") continue;

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
        controller.Init();

        // Move boss to initial location
        bossObject.GetComponent<wolf::Transform2D>()->SetPosition(m_bossfightPlayerPos + glm::vec2(0.0f, size.y * 0.5f));

        // Grab pointer to boss controller
        m_pBoss = &bossObject;

        break;
    }

    ItemDropCreator::CreateInstance(&scene, m_pLabyrinthManager->GetSeed());
    NPCBuilder::CreateInstance(&scene, m_pLabyrinthManager->GetSeed());

    // CreateThrowableObject();
    
    // Create a test spike trap
    CreateSpikeTrap(m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(192.0f, 192.0f));
    CreateBoulderTrap(m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(-192.0f, 192.0f));

    // Testing: Create a test projectile object
    // auto& testObj = scene.CreateObject2D();
    // testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));5
    // testObj.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(512.0f, 0.0f));
    // auto& testSprite = testObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    // auto& testCollider = testObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
    // testCollider.SetDamage(10.0f);
    // testCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    // auto& testVelocity = testObj.AddComponent<VelocityComponent>();
    //testVelocity.SetVelocity(glm::vec2(-128.0f, 0.0f));

    // auto& testObj2 = scene.CreateObject2D();
    // testObj2.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    // testObj2.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(6000.0f, 0.0f));

    // auto& testSprite2 = testObj2.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    // testSprite2.SetOriginToCenterOfTexture();

    // auto& testCollider2 = testObj2.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
    // testCollider2.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));
    
    // auto& testVelocity2 = testObj2.AddComponent<VelocityComponent>();
    // testVelocity2.SetVelocity(glm::vec2(0.0f, 128.0f));
    
    // auto& testHoming2 = testObj2.AddComponent<HomingComponent>(m_pPlayerObject, 1.0f);
    
    // this->CreateMinitaurEnemy();
    // this->CreateHarpyEnemy();
    // this->CreateGorgonEnemy();
    this->CreateTrappedChest();
}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueAndCutsceneEvent, PlayState, &PlayState::OnDialogueAndCutsceneTriggered>(*this);
    wolf::EventManager::RemoveListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);



    // Delete managers
    delete this->m_pColliderManager;
    this->m_pColliderManager = nullptr;
    
    
    DDACalculator::DestroyInstance();
    GLShapesRenderer::DestroyInstance();

    TileFireManager::DestroyInstance();

    ItemDropCreator::DestroyInstance();
    
    NPCBuilder::DestroyInstance();
}

void PlayState::Pause()
{
}

void PlayState::Resume()
{
}

void PlayState::Update(float delta)
{
    // Push the pause state when 'Escape' is pressed
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        m_pStateManager->PushState(new PauseState(m_pStateManager, m_pGameInstance));
    }

    // Toggle Labyrinth Manager GUI with the semicolon key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SEMICOLON)) 
        m_showLabyrinthManager = !m_showLabyrinthManager;
    
    // DEBUG: Teleport to bossfight
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT_SHIFT))
        m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetPosition(m_bossfightPlayerPos);

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
    
    // Update the labyrinth manager
    m_pLabyrinthManager->Update(delta);

    TileFireManager::GetInstance()->Update(delta);

    // Update timed destroyer components
    for (auto&& [_, TimedDestroyerComponent] : m_pGameInstance->GetScene().Each<TimedDestroyerComponent>())
    {
        TimedDestroyerComponent.Update(delta);
    }

    // INVENTORY TESTING
    auto* playerInventory = m_pPlayerObject->GetComponent<PlayerInventoryComponent>();
    if (playerInventory) {
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

            ItemBase* pBow = ItemCreator::CreateItem("Old Bow");
            ItemBase* pSpear = ItemCreator::CreateItem("Shaky Spear");

            ItemBase* pHealHeart = ItemCreator::CreateItem("Healing Heart");
            ItemBase* pHurtHeart = ItemCreator::CreateItem("Hurting Heart");
            ItemBase* pBurnHeart = ItemCreator::CreateItem("Burning Heart");
            
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

        playerInventory->ShowToggleButtonGUI();
        playerInventory->ShowInventoryGUI();
    }

    // DEBUG: Noclip hotkey
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SLASH))
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
                if (chestInventory.IsOpen())
                    sprite.SetAnimation(name.replace(name.find("Open"), 4, "Closed"));
                else
                    sprite.SetAnimation(name.replace(name.find("Closed"), 6, "Open"));
                
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
                auto name = sprite.GetCurrentAnimation()->m_strName;
                sprite.SetAnimation(name.replace(name.find("Open"), 4, "Closed"));
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

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_V)) {
            npc.QueueDialogue("test");
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
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_9))
    {
        // Trigger both cutscene and dialogue with IDs
        wolf::EventManager::TriggerEvent(DialogueAndCutsceneEvent("intro_sequence", "data/DialogueAndCutscenes.yaml"));
    }

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

    // Update damage indicators
    for (auto&& [_, health] : m_pGameInstance->GetScene().Each<HealthComponent>()) {
        health.UpdateDamageIndicators(delta);
    }

    // Dispatch events
    wolf::EventManager::Dispatch();

    // ImGui::ShowDemoWindow();
}

void PlayState::Render(float delta)
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render(delta);
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

    RenderMinimap();
}

void PlayState::BackgroundUpdate(float delta)
{
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
}

void PlayState::BackgroundRender(float delta)
{
    m_pGameInstance->GetScene().Render(delta);
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
    // Start player at the labyrinth spawn location and scale appropriately
    auto& transform = *m_pPlayerObject->GetComponent<wolf::Transform2D>();
    transform.SetScale(glm::vec2(3));
}

void PlayState::CreateMinitaurEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    MinitaurBuilder minitaurBuilder(m_pGameInstance->GetScene());

    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 240.0f); 

    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, position);
    
    // Set the scale of each Minitaur to 3
    auto* transform = minitaur.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each minitaur
    }

    auto* statusComponent = minitaur.GetComponent<StatusComponent>();
    // statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 4.0f);
}
void PlayState::CreateHarpyEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    HarpyBuilder harpyBuilder(m_pGameInstance->GetScene());
    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 360.0f);

    EnemyData harpyData = loader.LoadEnemyData("harpy");
    auto& harpy = harpyBuilder.BuildHarpy(harpyData, position);
    
    // Set the scale of each Minitaur to 3
    auto* transform = harpy.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each harpy
    }
    auto* statusComponent = harpy.GetComponent<StatusComponent>();
    // statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 2.0f);
}


void PlayState::CreateGorgonEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    GorgonBuilder gorgonBuilder(m_pGameInstance->GetScene());

    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 720.0f);

    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    auto& gorgon = gorgonBuilder.BuildGorgon(gorgonData, position);
    
    // Set the scale of each Gorgon to 3
    auto* transform = gorgon.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each gorgon
    }
    auto* statusComponent = gorgon.GetComponent<StatusComponent>();
    // statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 3.0f);
}

void PlayState::CreateTrappedChest()
{
    // Create the chest object
    wolf::GameObject* chest = &m_pGameInstance->GetScene().CreateObject2D();
    
    // Scale the chest
    auto& transform = *chest->GetComponent<wolf::Transform2D>();
    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 288.0f);
    transform.SetPosition(position);
    transform.SetScale(glm::vec2(3.0f));

    // Add the sprite
    auto& sprite = chest->AddComponent<AnimatedSprite2D>("data/chest_anim_init.yaml");
    sprite.SetAnimation("LegendaryClosed");
    sprite.SetOriginToCenterOfFrame();

    // Add the collider
    auto& collider = chest->AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true);
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16, 16));

    // Add the trapped chest component
    auto& trappedChestComp = chest->AddComponent<TrappedChestComponent>(TrappedChestComponent::TrapType::EXPLODE, true,  true);
    trappedChestComp.Init();
}

void PlayState::CreateThrowableObject()
{
    // Get the spawn location from the labyrinth manager
    glm::vec2 spawnLocation = m_pLabyrinthManager->GetSpawnLocation();

    // Create a throwable object in the scene
    auto& throwableObj = m_pGameInstance->GetScene().CreateObject2D();

    // Set the initial position based on the spawn location
    auto* transform = throwableObj.GetComponent<wolf::Transform2D>();
    if (transform) {
        transform->SetPosition(spawnLocation); // Set to labyrinth's spawn position
    } else {
        transform = &throwableObj.AddComponent<wolf::Transform2D>();
        transform->SetPosition(spawnLocation);
    }

    // Add a sprite for visual representation (optional)
    auto& sprite = throwableObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    sprite.SetOriginToCenterOfTexture();

    // Add a collider to enable interaction with enemies
    auto& collider = throwableObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 1, 0);
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));  // Adjusted size for the object

    // Add the velocity component with an initial zero velocity
    auto& velocity = throwableObj.AddComponent<VelocityComponent>();
    velocity.SetVelocity(glm::vec2(0.0f, 0.0f)); // Will be updated upon throwing

    // Add the throwable component with parameters matching the constructor
    auto& throwable = throwableObj.AddComponent<ThrowableObjectComponent>(25.0f, m_pColliderManager);
}

void PlayState::OnDialogueAndCutsceneTriggered(const DialogueAndCutsceneEvent& event) {
    std::cout << "Triggered sequence: " << event.sequenceID << std::endl;

    // Push the DialogueAndCutsceneState onto the game state stack
    auto* dialogueAndCutsceneState = new DialogueAndCutsceneState(m_pStateManager, m_pGameInstance, event.dialogueFilePath, event.triggerNPCID);
    dialogueAndCutsceneState->LoadSequence(event.sequenceID);  // Start the specific sequence
    m_pStateManager->PushState(dialogueAndCutsceneState);
}

wolf::GameObject& PlayState::CreateSpikeTrap(const glm::vec2& position)
{
    // Create the trap object
    auto& trap = m_pGameInstance->GetScene().CreateObject2D();

    // Add sprite
    auto& sprite = trap.AddComponent<wolf::Sprite2D>("data/textures/SpikesRetracted.png");
    sprite.SetOriginToCenterOfTexture();
    sprite.SetLayer(0);

    // Set position
    auto& transform = *trap.GetComponent<wolf::Transform2D>();
    transform.SetPosition(position);
    transform.SetScale(glm::vec2(3.0f));

    // Add a collider for interaction
    auto& collider = trap.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
    collider.AddColliderBox(glm::vec2(24.0f, 24.0f), glm::vec2(-12.0f, 12.0f));

    // Add the TriggerComponent
    trap.AddComponent<TriggerComponent>(m_pColliderManager, TriggerType::REUSABLE, TriggerPurpose::SPIKE_TRAP, EntityListenType::PLAYER_IGNORE_ROLLING);

    return trap;
}
wolf::GameObject& PlayState::CreateBoulderTrap(const glm::vec2& position)
{
    // Create the trap object
    auto& bouldertrap = m_pGameInstance->GetScene().CreateObject2D();

    // Add sprite
    auto& sprite = bouldertrap.AddComponent<wolf::Sprite2D>("data/textures/SpikesRetracted.png");
    sprite.SetOriginToCenterOfTexture();
    sprite.SetLayer(0);

    // Set position
    auto& transform = *bouldertrap.GetComponent<wolf::Transform2D>();
    transform.SetPosition(position);
    transform.SetScale(glm::vec2(3.0f));

    // Add a collider for interaction
    auto& collider = bouldertrap.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
    collider.AddColliderBox(glm::vec2(24.0f, 24.0f), glm::vec2(-12.0f, 12.0f));

    // Add the TriggerComponent
    bouldertrap.AddComponent<TriggerComponent>(m_pColliderManager, TriggerType::REUSABLE, TriggerPurpose::BOULDER_TRAP);

    return bouldertrap;
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
            auto& boulderObj = m_pGameInstance->GetScene().CreateObject2D();
            auto& boulderSprite = boulderObj.AddComponent<wolf::Sprite2D>("data/textures/Boulder.png");
            boulderSprite.SetOriginToCenterOfTexture();
            boulderSprite.SetLayer(1);
            auto* boulderTransform = boulderObj.GetComponent<wolf::Transform2D>();
            if (!boulderTransform) {
                boulderTransform = &boulderObj.AddComponent<wolf::Transform2D>();
            }
            // Set Position (spawn X + 192 units away, facing left)
            glm::vec2 spawnPosition = triggerPosition + glm::vec2(192.0f, 0.0f);
            boulderTransform->SetPosition(spawnPosition);
            boulderTransform->SetScale(glm::vec2(3.0f)); // Scale the boulder

            // Add Velocity Component (for optional movement logic)
            auto& velocity = boulderObj.AddComponent<VelocityComponent>();
            velocity.SetVelocity(glm::vec2(-100.0f, 0.0f)); // Initial velocity to move left

            // Add Collider for the Boulder
            auto& boulderCollider = boulderObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
            boulderCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f)); 

            boulderObj.AddComponent<BoulderTrapComponent>(triggerObject->GetComponent<TriggerComponent>(), m_pColliderManager, BoulderDirection::UP, 100.0f, 5.0f);

            // wolf::Log("Boulder trap triggered!");
            break;
        }
        case TriggerPurpose::BOSS: {
            // Begin the bossfight
            wolf::Log("BOSSFIGHT STARTED");
            m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetPosition(m_bossfightPlayerPos);
            m_pBoss->GetComponent<BossController>()->SetActive(true);
            break;
        }
        default:
            wolf::Log("Unsupported trigger purpose.");
            break;
    }
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
            return IM_COL32(60, 60, 60, 255); // Walls
        case Tile::BorderedGrass:
        case Tile::Bricks:
            return IM_COL32(180, 140, 90, 255); // Bricks/Grass
        case Tile::FloorSmallSquares:
            return IM_COL32(140, 140, 140, 255); // Small Squares
        case Tile::FloorSmallSquaresGold:
        case Tile::FloorSquareGold:
        case Tile::FloorSpiralGold:
            return IM_COL32(220, 180, 50, 255); // Gold Floors
        case Tile::FloorSquare:
        case Tile::FloorSpiral:
            return IM_COL32(160, 160, 160, 255); // Normal Floors
        case Tile::Grass:
            return IM_COL32(50, 180, 50, 255); // Grass
        default:
            return IM_COL32(100, 100, 100, 255); // Default Color
    }
}


void PlayState::RenderMinimap() {
    static float zoomScale = 1.0f;

    // Handle scroll wheel input for zooming
    if (m_isMapExpanded) {
        float scrollDelta = ImGui::GetIO().MouseWheel;
        zoomScale += scrollDelta * 0.1f; // Adjust zoom increment
        zoomScale = glm::clamp(zoomScale, 0.5f, 2.0f); // Limit zoom range
    }

    // Determine minimap configuration
    const float minimapRadius = m_isMapExpanded ? 300.0f : 100.0f;
    const float labyrinthScale = (m_isMapExpanded ? zoomScale : 0.2f);

    // Set the center of the map
    ImVec2 minimapCenter = m_isMapExpanded
        ? ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f)
        : ImVec2(ImGui::GetIO().DisplaySize.x - minimapRadius - 20.0f, 20.0f + minimapRadius);

    // Get player position and chunk
    const glm::vec2 playerPosition = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::ivec2 playerChunkID = m_pLabyrinthManager->GetChunkID(playerPosition);

    // Calculate visible chunks
    const int chunkRadius = std::ceil(minimapRadius / (LabyrinthManager::CHUNK_SIZE * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE));

    // Begin ImGui rendering
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(2 * minimapRadius, 2 * minimapRadius));
    ImGui::SetNextWindowPos(ImVec2(minimapCenter.x - minimapRadius, minimapCenter.y - minimapRadius));
    ImGui::Begin("Minimap###AlwaysVisible", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_NoBackground);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 center = ImVec2(windowPos.x + minimapRadius, windowPos.y + minimapRadius);
    glm::vec2 centerGLM = ToGLMVec2(center);

    // Clip rendering to the circular area
    drawList->PushClipRect(
        ImVec2(center.x - minimapRadius, center.y - minimapRadius),
        ImVec2(center.x + minimapRadius, center.y + minimapRadius),
        true // Intersect with existing clip rect
    );

    // Draw circular boundary
    drawList->AddCircleFilled(center, minimapRadius, IM_COL32(30, 30, 30, 220)); // Background
    drawList->AddCircle(center, minimapRadius, IM_COL32(255, 255, 255, 255), 64, 3.0f); // Border

    // Render visible chunks
    for (int cx = -chunkRadius; cx <= chunkRadius; ++cx) {
        for (int cy = -chunkRadius; cy <= chunkRadius; ++cy) {
            glm::ivec2 chunkID = playerChunkID + glm::ivec2(cx, cy);
            wolf::GameObject* chunk = m_pLabyrinthManager->GetChunk(chunkID);
            if (!chunk) continue; // Skip non-existent chunks

            // Render tiles within the chunk
            for (int tx = 0; tx < LabyrinthManager::CHUNK_SIZE; ++tx) {
                for (int ty = 0; ty < LabyrinthManager::CHUNK_SIZE; ++ty) {
                    glm::ivec2 tilePos = chunkID * LabyrinthManager::CHUNK_SIZE + glm::ivec2(tx, ty);
                    int tileID = m_pLabyrinthManager->GetTile(tilePos.x, tilePos.y);
                    if (tileID == Tile::Empty || tileID < 0) continue; // Skip invalid or empty tiles

                    // Calculate relative position
                    glm::vec2 tileWorldPos = glm::vec2(
                        tilePos.x * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE,
                        tilePos.y * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE
                    );
                    glm::vec2 relativePos = (tileWorldPos - playerPosition) * labyrinthScale;
                    relativePos.y = -relativePos.y; // Invert Y-axis for rendering
                    if (glm::length(relativePos) > minimapRadius) continue;

                    // Render tile
                    ImVec2 tileScreenPos = ToImVec2(centerGLM + relativePos);
                    float tileSize = 6.0f * labyrinthScale;
                    ImVec2 tileMin(tileScreenPos.x - tileSize / 2, tileScreenPos.y - tileSize / 2);
                    ImVec2 tileMax(tileScreenPos.x + tileSize / 2, tileScreenPos.y + tileSize / 2);
                    drawList->AddRectFilled(tileMin, tileMax, GetTileColor(tileID));
                }
            }
        }
    }

    // Draw player icon
    drawList->AddCircleFilled(center, 6.0f, IM_COL32(0, 255, 0, 255)); // Player icon

    // Render enemies
    for (auto&& [_, minitaurController] : m_pGameInstance->GetScene().Each<MinitaurController>()) {
        glm::vec2 minitaurPos = minitaurController.GetGameObject()
                                                ->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        glm::vec2 relativePos = (minitaurPos - playerPosition) * labyrinthScale;
        relativePos.y = -relativePos.y; // Invert Y-axis for proper rendering
        if (glm::length(relativePos) > minimapRadius) continue;

        // Render enemy
        ImVec2 enemyScreenPos = ToImVec2(centerGLM + relativePos);
        drawList->AddCircle(enemyScreenPos, 7.0f, IM_COL32(255, 50, 50, 100), 32, 2.0f); // Outer glow
        drawList->AddCircle(enemyScreenPos, 5.0f, IM_COL32(255, 100, 100, 150), 32, 2.0f); // Middle ring
        drawList->AddCircleFilled(enemyScreenPos, 3.0f, IM_COL32(255, 0, 0, 255));        // Inner core
    }

    // Pop the clip rect to restore normal rendering
    drawList->PopClipRect();
    ImGui::End();
    ImGui::PopStyleVar();
}