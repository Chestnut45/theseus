#include "PlayState.h"
#include "PauseState.h"
#include "DialogueState.h"
#include <imgui/imgui.h>

#include "../components/ColliderComponent.h"
#include "../components/HealthComponent.h"
#include "../components/PlayerInventoryComponent.h"
#include "../components/ChestInventoryComponent.h"
#include "../components/StatusComponent.h"
#include "../components/VelocityComponent.h"
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the listener
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

    m_pTriggerManager = new TriggerManager();
    
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


    auto& testObj2 = scene.CreateObject2D();
    testObj2.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    testObj2.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(5120.0f, 0.0f));

    auto& testSprite2 = testObj2.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    testSprite2.SetOriginToCenterOfTexture();

    auto& testCollider2 = testObj2.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
    testCollider2.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));
    
    auto& testVelocity2 = testObj2.AddComponent<VelocityComponent>();
    // testVelocity2.SetVelocity(glm::vec2(64.0f, 0.0f));
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
    for (auto&& [_, trigger] : m_pGameInstance->GetScene().Each<TriggerComponent>())
    {
        trigger.Update(delta); // No delta needed since it just checks for collisions
    }
    for (auto&& [_, trap] : m_pGameInstance->GetScene().Each<TrapComponent>())
    {
        trap.Update(delta);  // Manage lifespan, attack cooldown, and other logic here
    }

    // // Debugging the final minitaur's position and state

    // for (auto&& [_, minitaurController] : m_pGameInstance->GetScene().Each<MinitaurController>())
    // {
    //     auto* pGameObject = minitaurController.GetGameObject();
    //     if (pGameObject)
    //     {
    //         auto* transform = pGameObject->GetComponent<wolf::Transform2D>();
    //         if (transform)
    //         {
    //             glm::vec2 pos = transform->GetGlobalPosition();
    //             printf("Minitaur Render Position: (%f, %f)\n", pos.x, pos.y);  // Debug rendering position
    //         }
    //     }
    // }

    
    // Update all animated sprites
    for (auto&&[_, anim] : m_pGameInstance->GetScene().Each<AnimatedSprite2D>())
    {
        anim.Update(delta);
    }

    // Update collisions
    this->m_pColliderManager->Update(delta);

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())
    {
        transform.Translate(velocity.GetVelocity() * delta);
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

    // Inflict status effects upon the player
    for (auto&& [_, status] : m_pGameInstance->GetScene().Each<StatusComponent>())
    {
        status.Update();
    }

    if (wolf::Input::IsKeyJustDown(GLFW_KEY_9))
    {
        // Broadcast the DialogueTriggerEvent with a specific dialogue ID
        wolf::EventManager::TriggerEvent(DialogueTriggerEvent("intro_1"));
    }
    m_pTriggerManager->Update(delta);
    wolf::EventManager::Dispatch<DialogueTriggerEvent>();
    
    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);

    // Update managers
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
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

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
    if (event.triggerName == "PressurePlateSteppedOn") {
        // Get the position of the pressure plate (assuming the event holds the trigger object reference)
        auto* pressurePlateObject = event.triggerObject;

        if (pressurePlateObject) {
            auto* plateTransform = pressurePlateObject->GetComponent<wolf::Transform2D>();
            if (plateTransform) {
                // Get the global position of the pressure plate
                glm::vec2 platePosition = plateTransform->GetGlobalPosition();
                
                // Adjust the trap's spawn position to be slightly in front of the pressure plate (e.g., 50 units in the Y direction)
                glm::vec2 trapOffset = glm::vec2(0.0f, -64.0f); // Adjust offset as needed
                glm::vec2 trapPosition = platePosition + trapOffset;

                // Create the trap object
                auto& trapObj = m_pGameInstance->GetScene().CreateObject2D();

                // Add trap sprite for visualization
                auto& trapSprite = trapObj.AddComponent<wolf::Sprite2D>("data/textures/spiketrap.png");
                trapSprite.SetOriginToCenterOfTexture();

                // Add transform and set the calculated position
                if (!trapObj.HasAll<wolf::Transform2D>()) {
                    trapObj.AddComponent<wolf::Transform2D>();
                   
                }
                auto* trapTransform = trapObj.GetComponent<wolf::Transform2D>();
                trapTransform->SetPosition(trapPosition);
                trapTransform->SetScale(glm::vec2(3.0f));  // Scale the trap's sprite by 3

                // Add velocity component (even if trap is stationary)
                auto& velocity = trapObj.AddComponent<VelocityComponent>();
                velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

                // Add the TrapComponent with 50 damage, 5-second lifespan, and the collider manager
                auto& trap = trapObj.AddComponent<TrapComponent>(50.0f, 5.0f, m_pColliderManager);

                // Add a collider to detect collisions with the player
                auto& trapCollider = trapObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
                trapCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));

                // Activate the trap immediately
                trap.Activate();
            }
        }
    }
}

void PlayState::CreatePressurePlate() {
    // Create the pressure plate object
    auto& pressurePlateObj = m_pGameInstance->GetScene().CreateObject2D();
    // Add a sprite for visualization
    auto& sprite = pressurePlateObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/pressureplate.png");
    sprite.SetOriginToCenterOfTexture();
    // Add a transform component and set the position
    if (!pressurePlateObj.HasAll<wolf::Transform2D>()) {
        pressurePlateObj.AddComponent<wolf::Transform2D>();
    }
    auto* transform =  pressurePlateObj.GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(100.0f,100.0f));
    transform->SetScale(glm::vec2(3.0f));
        

    


    auto& velocity = pressurePlateObj.AddComponent<VelocityComponent>();
    velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

    // Add a collider for interaction
    auto& collider = pressurePlateObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));

    // Add the trigger component for the pressure plate
    auto& pressurePlate = pressurePlateObj.AddComponent<TriggerComponent>(m_pColliderManager, "PressurePlateSteppedOn");

    // Add the trigger to the TriggerManager
    m_pTriggerManager->AddTrigger(&pressurePlate);
}
