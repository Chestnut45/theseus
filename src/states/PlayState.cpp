#include "PlayState.h"
#include "PauseState.h"
#include "DialogueState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HitboxComponent.h"
#include "../components/HurtboxComponent.h"
#include "../components/VelocityComponent.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize hitbox / hurtbox managers
    this->m_pHitboxManager = new HitboxManager(&scene);
    this->m_pHurtboxManager = new HurtboxManager(&scene);

    // Initialize the listener
    wolf::EventManager::AddListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);

    // Initialize player object
    CreatePlayer();

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager component to an empty object and load default config
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();
    m_pLabyrinthManager->LoadConfig("data/labyrinth_config.yaml");

    // Testing: Create a test projectile object
    auto& testObj = scene.CreateObject2D();
    testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));
    testObj.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(256.0f, 0.0f));
    auto& testSprite = testObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");

    // Initialize test hitbox and hurtbox components
    auto& testHitbox = testObj.AddComponent<HitboxComponent>(1, 1);
    testHitbox.AddHitbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));

    auto& testHurtbox = testObj.AddComponent<HurtboxComponent>(1, 1, 1, 1);
    testHurtbox.AddHurtbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));

    // Set velocity component
    auto& testVelocity = testObj.AddComponent<VelocityComponent>();
    testVelocity.SetVelocity(glm::vec2(-32.0f, 0.0f));
}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);

    // Delete managers
    delete this->m_pHitboxManager;
    this->m_pHitboxManager = nullptr;
    delete this->m_pHurtboxManager;
    this->m_pHurtboxManager = nullptr;
}

void PlayState::Pause()
{
}

void PlayState::Resume()
{
}

void PlayState::Update(float delta)
{
    // Input and debug hotkey handling

    // Push the pause state when 'Escape' is pressed
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        m_pStateManager->PushState(new PauseState(m_pStateManager, m_pGameInstance));
    }

    // Toggle Labyrinth Manager GUI with the semicolon key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SEMICOLON)) m_showLabyrinthManager = !m_showLabyrinthManager;

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();

    // Main object / component updates

    // Update player controller
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) playerController->Update(delta);

    // Update player animations
    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) {
        playerAnim->Update(delta);
    }

    // INVENTORY TESTING
    auto* playerInventory = m_pPlayerObject->GetComponent<InventoryComponent>();
    if (playerInventory) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_Q)) m_showInventoryGUI = !m_showInventoryGUI;
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
            ItemBase* pAddItem = new ConsumableItem(CONSUMABLE, "Stacking Heart", "This is a test consumable item that stacks", 10, true, 1);
            playerInventory->AddItem(pAddItem);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_4)) {
            ItemBase* pAddItem = new ConsumableItem(CONSUMABLE, "Multi-Use Heart", "This is a test consumable item with multiple uses", 25, false, 3);
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
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())  // Use GetScene()
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }

    if (wolf::Input::IsKeyJustDown(GLFW_KEY_E))
    {
        // Broadcast the DialogueTriggerEvent with a specific dialogue ID
        wolf::EventManager::TriggerEvent(DialogueTriggerEvent("intro_1"));
    }
    wolf::EventManager::Dispatch<DialogueTriggerEvent>();
    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);

    // Update managers
    this->m_pHitboxManager->Update();
    this->m_pHurtboxManager->Update();
    wolf::EventManager::Dispatch<DialogueTriggerEvent>();
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
    // Update logic for when state is inactive

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();
}

void PlayState::BackgroundRender()
{
    // Render logic for when state is inactive
    
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
}

void PlayState::CreatePlayer()
{
    // Create player object with transform
    m_pPlayerObject = &m_pGameInstance->GetScene().CreateObject2D();

    // Add player controller
    m_pPlayerObject->AddComponent<PlayerController>();

    // Scale player
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add animated sprite
    auto& animSprite = m_pPlayerObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
    animSprite.AddAnimation("WalkSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, true);
    animSprite.AddAnimation("WalkEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, true);
    animSprite.AddAnimation("WalkNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, true);
    animSprite.AddAnimation("WalkWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, true);
    animSprite.AddAnimation("StandSouth", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    animSprite.AddAnimation("StandEast", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 2, 2, false);
    animSprite.AddAnimation("StandNorth", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 3, 3, false);
    animSprite.AddAnimation("StandWest", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 4, 4, false);
    animSprite.SetAnimation("StandSouth");
    animSprite.SetOriginToCenterOfFrame();

    // Add velocity
    m_pPlayerObject->AddComponent<VelocityComponent>();

    // INVENTORY TESTING
    auto& inventory = m_pPlayerObject->AddComponent<InventoryComponent>(16, 4, "data/textures/DebugSprites/TestItems.png", glm::vec2(32.0f, 32.0f));

    // Add hitbox
    auto& hitbox = m_pPlayerObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add hurtbox
    auto& hurtbox = m_pPlayerObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add health / armor
    m_pPlayerObject->AddComponent<HealthComponent>();
    m_pPlayerObject->AddComponent<ArmourComponent>(50);
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