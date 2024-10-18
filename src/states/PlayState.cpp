#include "PlayState.h"
#include "PauseState.h"
#include "DialogueState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/ColliderComponent.h"
#include "../components/HealthComponent.h"
#include "../components/InventoryComponent.h"
#include "../components/StatusComponent.h"
#include "../components/VelocityComponent.h"


void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the listener
    wolf::EventManager::AddListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);

    // Initialize the listener
    wolf::EventManager::AddListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);
    this->m_pColliderManager = new ColliderManager(&scene);

    // Initialize the player object first
     CreatePlayer();


    // Initialize the Minitaur enemy object second
     CreateMinitaurEnemy();



    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Initialise managers
    

    // Add the labyrinth manager component to an empty object and load default config
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();
    m_pLabyrinthManager->LoadConfig("data/labyrinth_config.yaml");

    // Testing: Create a test projectile object
    auto& testObj = scene.CreateObject2D();
    testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    testObj.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(512.0f, 0.0f));
    auto& testSprite = testObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    auto& testCollider = testObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITBOX, 0, 1);
    testCollider.SetDamage(10.0f);
    testCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testVelocity = testObj.AddComponent<VelocityComponent>();
    //testVelocity.SetVelocity(glm::vec2(-128.0f, 0.0f));


    // auto& testObj2 = scene.CreateObject2D();
    // testObj2.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    // testObj2.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(256.0f, 0.0f));
    // auto& testSprite2 = testObj2.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    // auto& testCollider2 = testObj2.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDD, 1, 1);
    // testCollider2.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    // auto& testVelocity2 = testObj2.AddComponent<VelocityComponent>();
    // testVelocity2.SetVelocity(glm::vec2(64.0f, 0.0f));

}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);

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

    // Main object / component updates
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) 
        playerController->Update(delta);

    // Update managers
    this->m_pColliderManager->Update(delta);

    // Update player animations
    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) 
        playerAnim->Update(delta);

    auto* minitaurController = m_pMinitaurObject->GetComponent<MinitaurController>();
    if (minitaurController)
    {
        minitaurController->Update(delta);
    }
    auto* enemyAnim = m_pMinitaurObject->GetComponent<AnimatedSprite2D>();
     if (enemyAnim) 
        enemyAnim->Update(delta);
    
    // INVENTORY TESTING
    auto* playerInventory = m_pPlayerObject->GetComponent<InventoryComponent>();
    if (playerInventory) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_0)) m_showInventoryGUI = !m_showInventoryGUI;
        if (m_showInventoryGUI) playerInventory->ShowInventoryGUI();

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_1)) {
            ItemBase* pAddItem = new EquipmentItem(EQUIPMENT, "Test Helmet", "This is a test equipment item", 5, HEAD);
            playerInventory->AddItem(pAddItem);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_2)) {
            ItemBase* pAddItem = new EquipmentItem(EQUIPMENT, "Test Sword", "This is a different test equipment item", 10, WEAPON);
            playerInventory->AddItem(pAddItem);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_3)) {
            ItemBase* pAddItem = new FlatAmtItem(CONSUMABLE, "HEAL", "This is a test consumable item that heals the player", 10, false, 1, HEALTH, 25.0f);
            playerInventory->AddItem(pAddItem);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_4)) {
            ItemBase* pAddItem = new StatusEffectItem(CONSUMABLE, "BURN", "This is a test consumable item that applies the BURNING status effect", 25, false, 1, StatusComponent::BURNING, 2.0f);
            playerInventory->AddItem(pAddItem);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_5)) {
            ItemBase* pHeadItem = playerInventory->GetEquippedItem(HEAD);
            ItemBase* pWeaponItem = playerInventory->GetEquippedItem(WEAPON);
            if (pHeadItem) {
                printf("%s is equipped in the HEAD slot!\n", pHeadItem->GetName().c_str());
            }
            else {
                printf("Nothing is equipped in the HEAD slot!\n");
            }

            if (pWeaponItem) {
                printf("%s is equipped in the WEAPON slot!\n", pWeaponItem->GetName().c_str());
            }
            else {
                printf("Nothing is equipped in the WEAPON slot!\n");
            }
        }
    }

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }   

    if (wolf::Input::IsKeyJustDown(GLFW_KEY_9))
    {
        // Broadcast the DialogueTriggerEvent with a specific dialogue ID
        wolf::EventManager::TriggerEvent(DialogueTriggerEvent("intro_1"));
    }

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

    // Scale player
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add velocity
    m_pPlayerObject->AddComponent<VelocityComponent>();

    // Add inventory
    auto& inventory = m_pPlayerObject->AddComponent<InventoryComponent>(16, 4, "data/textures/DebugSprites/TestItems.png", glm::vec2(32.0f, 32.0f));

    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add health / armor
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(1000);
    auto& armour = m_pPlayerObject->AddComponent<ArmourComponent>();
    armour.CollectArmour(50, {{ArmourComponent::SpecialProperty::FIRERESISTANCE, 50}});
}

void PlayState::CreateMinitaurEnemy()
{
    // Instantiate the MinitaurBuilder with the current scene
    MinitaurBuilder minitaurBuilder(m_pGameInstance->GetScene());

    // Build the Minitaur and assign it to m_pMinitaurObject
    m_pMinitaurObject = &minitaurBuilder.BuildMinitaur(m_pColliderManager); 
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