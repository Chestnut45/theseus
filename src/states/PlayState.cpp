#include "PlayState.h"
#include "PauseState.h"
#include "DialogueState.h"
#include <imgui/imgui.h>

#include "../components/ChestInventoryComponent.h"
#include "../components/ColliderComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HomingComponent.h"
#include "../components/PlayerInventoryComponent.h"
#include "../components/AttackDamageComponent.h"
#include "../components/StatusComponent.h"
#include "../components/TimedDestroyerComponent.h"
#include "../components/VelocityComponent.h"
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the dialogue listener
    wolf::EventManager::AddListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);

    
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
    
    // Create the pressure plate using the helper function
    CreatePressurePlate();


    // Add the labyrinth manager and generate the default labyrinth config
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();
    m_pLabyrinthManager->m_pColliderManager = m_pColliderManager;
    m_pLabyrinthManager->LoadConfig("data/labyrinth_config.yaml");
    m_pLabyrinthManager->GenerateLabyrinth();

    // Testing: Create a test projectile object
    // auto& testObj = scene.CreateObject2D();
    // testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
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
}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);

    // Delete managers
    delete this->m_pColliderManager;
    this->m_pColliderManager = nullptr;
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

    // TESTING: Delete all tiles the player steps on
    // TODO: Check for floor tiles, change them to gold variant
    // const auto& pos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    // const auto& tilePos = m_pLabyrinthManager->GetTilePosition(pos);
    // if (m_pLabyrinthManager->GetTile(tilePos.x, tilePos.y) >= 0)
    // {
    //     m_pLabyrinthManager->SetTile(tilePos.x, tilePos.y, -1);
    // }

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
    for (auto&& [_, trigger] : m_pGameInstance->GetScene().Each<TriggerComponent>()) {
        trigger.Update(delta);
    }
    for (auto&& [_, trap] : m_pGameInstance->GetScene().Each<TrapComponent>()) {
        trap.Update(delta);
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

    // Update collisions
    this->m_pColliderManager->Update(delta);

    // Inflict status effects upon the player
    for (auto&& [_, status] : m_pGameInstance->GetScene().Each<StatusComponent>())
    {
        status.Update();
    }

    // INVENTORY TESTING
    auto* playerInventory = m_pPlayerObject->GetComponent<PlayerInventoryComponent>();
    if (playerInventory) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_0)) playerInventory->ToggleOpen();

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_1)) {
            ItemBase* pBoots = ItemCreator::CreateItem("The Floor is Lava Boots");
            ItemBase* pBow = ItemCreator::CreateItem("Old Bow");
            ItemBase* pHealHeart = ItemCreator::CreateItem("Healing Heart");
            ItemBase* pHurtHeart = ItemCreator::CreateItem("Hurting Heart");
            ItemBase* pBurnHeart = ItemCreator::CreateItem("Burning Heart");
            playerInventory->AddItemOrDelete(pBoots);
            playerInventory->AddItemOrDelete(pBow);
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

        playerInventory->ShowInventoryGUI();
    }
    
    auto* chest = m_pPlayerObject->GetComponent<ChestInventoryComponent>();
    if (chest) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_4)) {
            chest->ToggleOpen();
        }
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_5)) {
            chest->FillChestFromFile("data/test_chest_contents.yaml");
        }
        
        chest->ShowInventoryGUI();
    }

    if (wolf::Input::IsKeyJustDown(GLFW_KEY_9))
    {
        // Broadcast the DialogueTriggerEvent with a specific dialogue ID
        wolf::EventManager::TriggerEvent(DialogueTriggerEvent("intro_1"));
    }

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }
    
    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);

    // Dispatch events
    wolf::EventManager::Dispatch<DialogueTriggerEvent>();
    wolf::EventManager::Dispatch();
}

void PlayState::Render()
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController)
        playerController->Render();
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
    auto& inventory = m_pPlayerObject->AddComponent<PlayerInventoryComponent>(16, 4, "data/textures/DebugSprites/TestItems.png", glm::vec2(32.0f, 32.0f));

    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(13.0f, 27.0f), glm::vec2(-7.0f, 13.0f));

    // Add health
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(1000);

    // Add status component and status effect
    auto& status = m_pPlayerObject->AddComponent<StatusComponent>();
    // status.AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, -1.0f);

    m_pPlayerObject->AddComponent<ChestInventoryComponent>(4, 4, "data/textures/DebugSprites/TestItems.png", glm::vec2(32.0f, 32.0f));
}

void PlayState::CreateMinitaurEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    MinitaurBuilder minitaurBuilder(m_pGameInstance->GetScene());

    glm::vec2 positions[] = {
        glm::vec2(300.0f, 200.0f),
        glm::vec2(400.0f, 200.0f),
        glm::vec2(500.0f, 200.0f)
    };

    for (const auto& position : positions)
    {
        EnemyData minitaurData = loader.LoadEnemyData("minitaur");
        auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, position, m_pColliderManager);
        
        // Set the scale of each Minitaur to 3
        auto* transform = minitaur.GetComponent<wolf::Transform2D>();
        if (transform)
        {
            transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each minitaur
        }
    }
}
void PlayState::CreateHarpyEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    HarpyBuilder harpyBuilder(m_pGameInstance->GetScene());

    glm::vec2 positions[] = {
        glm::vec2(-300.0f, -300.0f),
        glm::vec2(-400.0f, -400.0f),
        glm::vec2(-500.0f, -500.0f)
    };

    for (const auto& position : positions)
    {
        EnemyData harpyData = loader.LoadEnemyData("harpy");
        auto& harpy = harpyBuilder.BuildHarpy(harpyData, position, m_pColliderManager);
        
        // Set the scale of each Minitaur to 3
        auto* transform = harpy.GetComponent<wolf::Transform2D>();
        if (transform)
        {
            transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each harpy
        }
    }
}

void PlayState::StartDialogue(const std::string& dialogueID)
{
    // Create a new DialogueState and push it onto the state stack
    DialogueState* dialogueState = new DialogueState(m_pStateManager, m_pGameInstance, m_pDialogueManager);
    m_pStateManager->PushState(dialogueState);

    // Start the dialogue with the given ID
    dialogueState->StartDialogue(dialogueID);
}

void PlayState::OnDialogueTriggerEvent(const DialogueTriggerEvent& event)
{
    StartDialogue(event.dialogueID);
}

void PlayState::OnTriggerEvent(const TriggerEvent& event) {
    wolf::Log("TriggerEvent received: " + event.triggerName);

    if (event.triggerName == "PressurePlateSteppedOn") {
        auto* pressurePlateObject = event.triggerObject;

        if (pressurePlateObject) {
            // Proceed with trap creation (no need to check or set trapSpawned here)
            auto* plateTransform = pressurePlateObject->GetComponent<wolf::Transform2D>();
            if (!plateTransform) {
                wolf::Error("PressurePlate has no Transform2D component!");
                return;
            }

            glm::vec2 platePosition = plateTransform->GetGlobalPosition();
            wolf::Log("PressurePlate position: (" + std::to_string(platePosition.x) + ", " + std::to_string(platePosition.y) + ")");

            glm::vec2 trapOffset = glm::vec2(0.0f, -64.0f); 
            glm::vec2 trapPosition = platePosition + trapOffset;

            // Create the trap object
            wolf::Log("Creating trap at position: (" + std::to_string(trapPosition.x) + ", " + std::to_string(trapPosition.y) + ")");
            auto& trapObj = m_pGameInstance->GetScene().CreateObject2D();

            // Add trap sprite
            auto& trapSprite = trapObj.AddComponent<wolf::Sprite2D>("data/textures/spiketrap.png");
            trapSprite.SetOriginToCenterOfTexture();

            // Add transform and set position
            if (!trapObj.HasAll<wolf::Transform2D>()) {
                wolf::Log("Adding Transform2D to trap...");
                trapObj.AddComponent<wolf::Transform2D>();
            }

            auto* trapTransform = trapObj.GetComponent<wolf::Transform2D>();
            trapTransform->SetPosition(trapPosition);
            trapTransform->SetScale(glm::vec2(3.0f)); 

            auto& velocity = trapObj.AddComponent<VelocityComponent>();
            velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

            auto& trap = trapObj.AddComponent<TrapComponent>(50.0f, 5.0f, m_pColliderManager);
            wolf::Log("Added TrapComponent with damage: 50.0 and lifespan: 5.0");

            auto& trapCollider = trapObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
            trapCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));
            wolf::Log("Added ColliderComponent to the trap.");

            trap.Activate();
            wolf::Log("Trap activated.");
        }
    }
}


void PlayState::CreatePressurePlate() {
    // Create the pressure plate object
    auto& pressurePlateObj = m_pGameInstance->GetScene().CreateObject2D();
    
    // Add a sprite for visualization
    auto& sprite = pressurePlateObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/pressureplate.png");
    sprite.SetOriginToCenterOfTexture();

    // Set up the transform and position
    if (!pressurePlateObj.HasAll<wolf::Transform2D>()) {
        pressurePlateObj.AddComponent<wolf::Transform2D>();
    }
    auto* transform = pressurePlateObj.GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(100.0f, 100.0f));
    transform->SetScale(glm::vec2(3.0f));

    // Add velocity component
    auto& velocity = pressurePlateObj.AddComponent<VelocityComponent>();
    velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

    // Add a collider for interaction
    auto& collider = pressurePlateObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));

    // Add TriggerComponent directly without TriggerManager
    pressurePlateObj.AddComponent<TriggerComponent>(m_pColliderManager);
}
