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
#include "../components/VelocityComponent.h"
#include "../components/ThrowableObjectComponent.h"
#include "../components/BoulderTrapComponent.h"
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"
#include "GLShapesRenderer.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the dialogue listener
    wolf::EventManager::AddListener<DialogueAndCutsceneEvent, PlayState, &PlayState::OnDialogueAndCutsceneTriggered>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::AddListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);
 
    this->m_pColliderManager = new ColliderManager(&scene);

    GLShapesRenderer::CreateInstance();

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

    ItemDropCreator::CreateInstance(&scene, m_pLabyrinthManager->GetSeed());

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
    
    GLShapesRenderer::DestroyInstance();

    ItemDropCreator::DestroyInstance();
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

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
    
    // Update the labyrinth manager
    m_pLabyrinthManager->Update(delta);

    // Update timed destroyer components
    for (auto&& [_, TimedDestroyerComponent] : m_pGameInstance->GetScene().Each<TimedDestroyerComponent>())
    {
        TimedDestroyerComponent.Update(delta);
    }

    // Update all player controllers
    for (auto&&[_, controller] : m_pGameInstance->GetScene().Each<PlayerController>())
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


    // Inflict status effects upon the player
    for (auto&& [_, status] : m_pGameInstance->GetScene().Each<StatusComponent>())
    {
        status.Update(delta);
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

    auto* merchant = m_pPlayerObject->GetComponent<MerchantInventoryComponent>();
    if (merchant) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_4)) {
            merchant->ToggleOpen();
        }
        merchant->ShowInventoryGUI();
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

    // Trigger CutsceneDialogueEvent when pressing 9
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_9))
    {
        // Trigger both cutscene and dialogue with IDs
        wolf::EventManager::TriggerEvent(DialogueAndCutsceneEvent("intro_sequence"));
    }

        // Update velocity components to apply friction and decelerate objects
    for (auto&& [_, velocity] : m_pGameInstance->GetScene().Each<VelocityComponent>()) {
        velocity.Update(delta);  // Update velocity with friction and other forces
    }

    // Update collisions
    this->m_pColliderManager->Update(delta);
    
    // Apply velocity for all objects with Transform2D and VelocityComponent
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>()) {
        transform.Translate(velocity.GetVelocity() * delta);
    }
    ConvertPlayerTileToGold();
    
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

void PlayState::Render()
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController)
        playerController->Render();
    
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
}

void PlayState::BackgroundUpdate(float delta)
{
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
}

void PlayState::BackgroundRender()
{
    m_pGameInstance->GetScene().Render();
}

void PlayState::CreatePlayer()
{
    // Create player object with transform
    m_pPlayerObject = &m_pGameInstance->GetScene().CreateObject2D();
    m_pGameInstance->GetScene().SetPlayerID(m_pPlayerObject->GetID());
    
    // Register the player (Theseus) in the shared context
    m_pGameInstance->GetSharedContext().RegisterEntity("Theseus", m_pPlayerObject->GetID());

    // Add player controller and initialize
    // NOTE: This manages all player animations and the animated sprite component for the player
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();
    playerController.LateInitialize();
    // Start player at the labyrinth spawn location and scale appropriately
    auto& transform = *m_pPlayerObject->GetComponent<wolf::Transform2D>();
    transform.SetScale(glm::vec2(3));

    // Add velocity
    m_pPlayerObject->AddComponent<VelocityComponent>();

    // Add inventory
    auto& inventory = m_pPlayerObject->AddComponent<PlayerInventoryComponent>(16, 4, ImVec2(50, 50));

    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(7.0f, 8.0f), glm::vec2(-4.0f, -4.0f));

    // Add health
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(1000);

    // Add status component and status effect
    auto& status = m_pPlayerObject->AddComponent<StatusComponent>();


    // status.AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 3.0f);
    // status.AddStatusEffect(StatusComponent::StatusEffectType::POISONED, 5.0f);
    // status.AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 7.0f);

    // !-- THESE ARE TEST COMPONENTS FOR THE OTHER INVENTORY SYSTEMS. REMOVE THEM LATER --!
    MerchantInventoryComponent* pMerchant = &m_pPlayerObject->AddComponent<MerchantInventoryComponent>(16, 4, ImVec2(800, 200), "Merchant Guy", 0.1f, 50);
    pMerchant->FillInventoryFromFile("data/test_chest_contents.yaml");
}

void PlayState::CreateMinitaurEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    MinitaurBuilder minitaurBuilder(m_pGameInstance->GetScene());

    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 240.0f); 

    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, position, m_pColliderManager);
    
    // Set the scale of each Minitaur to 3
    auto* transform = minitaur.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each minitaur
    }

    auto* statusComponent = minitaur.GetComponent<StatusComponent>();
    // statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 1.0f);
}
void PlayState::CreateHarpyEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    HarpyBuilder harpyBuilder(m_pGameInstance->GetScene());
    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 360.0f);

    EnemyData harpyData = loader.LoadEnemyData("harpy");
    auto& harpy = harpyBuilder.BuildHarpy(harpyData, position, m_pColliderManager);
    
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

    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 580.0f);

    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    auto& gorgon = gorgonBuilder.BuildGorgon(gorgonData, position, m_pColliderManager);
    
    // Set the scale of each Gorgon to 3
    auto* transform = gorgon.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each gorgon
    }
    auto* statusComponent = gorgon.GetComponent<StatusComponent>();
    // statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 3.0f);
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
    auto* dialogueAndCutsceneState = new DialogueAndCutsceneState(m_pStateManager, m_pGameInstance, "data/DialogueAndCutscenes.yaml");
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
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));

    // Add the TriggerComponent
    trap.AddComponent<TriggerComponent>(m_pColliderManager, TriggerType::REUSABLE, TriggerPurpose::SPIKE_TRAP);

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
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));

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
            trapCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));
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



