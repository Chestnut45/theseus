#include "PlayState.h"
#include "PauseState.h"
#include "CutSceneState.h"
#include "DialogueState.h"
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
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialize the dialogue listener
    wolf::EventManager::AddListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::AddListener<TriggerEvent, PlayState, &PlayState::OnCutsceneTriggerEvent>(*this);

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

    CreateThrowableObject();
    
CreatePressurePlate(m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(96.0f, 96.0f), TriggerType::SINGLE_USE, TrapType::SPIKE_TRAP);
CreatePressurePlate(m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(192.0f, 192.0f), TriggerType::REUSABLE, TrapType::BOULDER_TRAP);
CreatePressurePlate(m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(-192.0f, -192.0f), TriggerType::CUTSCENE_SINGLE);


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
    this->CreateHarpyEnemy();
}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    wolf::EventManager::RemoveListener<DialogueTriggerEvent, PlayState, &PlayState::OnDialogueTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<TriggerEvent, PlayState, &PlayState::OnTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<TriggerEvent, PlayState, &PlayState::OnCutsceneTriggerEvent>(*this);
    wolf::EventManager::RemoveListener<GameOverEvent, PlayState, &PlayState::OnGameOverEvent>(*this);

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
    
    // Update the labyrinth manager
    m_pLabyrinthManager->Update(delta);

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

    // Display all open chest GUIs
    const auto& playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    for (auto&&[_, chestInventory, transform] : m_pGameInstance->GetScene().Each<ChestInventoryComponent, wolf::Transform2D>())
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

    auto* dispensary = m_pPlayerObject->GetComponent<DispensaryInventoryComponent>();
    if (dispensary) {
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_5)) {
            dispensary->ToggleOpen();
        }

        dispensary->ShowInventoryGUI();
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
    ConvertPlayerTileToGold();
    
    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);

    // Dispatch events
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
    auto& inventory = m_pPlayerObject->AddComponent<PlayerInventoryComponent>(16, 4, ImVec2(500, 200));

    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(7.0f, 8.0f), glm::vec2(-4.0f, -4.0f));

    // Add health
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(1000);

    // Add status component and status effect
    auto& status = m_pPlayerObject->AddComponent<StatusComponent>();
    // status.AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, -1.0f);

    // !-- THESE ARE TEST COMPONENTS FOR THE OTHER INVENTORY SYSTEMS. REMOVE THEM LATER --!
    MerchantInventoryComponent* pMerchant = &m_pPlayerObject->AddComponent<MerchantInventoryComponent>(16, 4, ImVec2(800, 200), "Merchant Guy", 0.1f, 50);
    pMerchant->FillInventoryFromFile("data/test_chest_contents.yaml");
    DispensaryInventoryComponent* pDispensary = &m_pPlayerObject->AddComponent<DispensaryInventoryComponent>(16, 4, ImVec2(200, 200));
    pDispensary->FillInventoryFromFile("data/test_dispensary_contents.yaml");
}

void PlayState::CreateMinitaurEnemy()
{
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    MinitaurBuilder minitaurBuilder(m_pGameInstance->GetScene());

    glm::vec2 positions[] = {
        // glm::vec2(300.0f, 200.0f),
        // glm::vec2(400.0f, 200.0f),
        // glm::vec2(500.0f, 200.0f)
        glm::vec2(6000.0f, 0.0f)
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
    glm::vec2 position = m_pLabyrinthManager->GetSpawnLocation() + glm::vec2(0.0f, 360.0f);

    EnemyData harpyData = loader.LoadEnemyData("harpy");
    auto& harpy = harpyBuilder.BuildHarpy(harpyData, position, m_pColliderManager);
    
    // Set the scale of each Minitaur to 3
    auto* transform = harpy.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));  // Set uniform scale to 3 for each harpy
    }
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

void PlayState::CreatePressurePlate(const glm::vec2& position, TriggerType triggerType, TrapType trapType) {
    // Create the pressure plate object in the scene
    auto& pressurePlateObj = m_pGameInstance->GetScene().CreateObject2D();

    // Add a sprite for visualization
    auto& sprite = pressurePlateObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/pressureplate.png");
    sprite.SetOriginToCenterOfTexture();

    // Check if the Transform2D component exists, and add it if not
    if (!pressurePlateObj.HasAll<wolf::Transform2D>()) {
        pressurePlateObj.AddComponent<wolf::Transform2D>();
    }
    auto* transform = pressurePlateObj.GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);
    transform->SetScale(glm::vec2(3.0f));

    // Add velocity component (optional if no movement is needed)
    if (!pressurePlateObj.HasAll<VelocityComponent>()) {
        pressurePlateObj.AddComponent<VelocityComponent>();
    }
    auto* velocity = pressurePlateObj.GetComponent<VelocityComponent>();
    velocity->SetVelocity(glm::vec2(0.0f, 0.0f));

    // Add a collider for interaction
    if (!pressurePlateObj.HasAll<ColliderComponent>()) {
        pressurePlateObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
    }
    auto* collider = pressurePlateObj.GetComponent<ColliderComponent>();
    collider->AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));

    // Add the TriggerComponent with specified trigger and trap types
    pressurePlateObj.AddComponent<TriggerComponent>(m_pColliderManager, triggerType, trapType);

    // Log the creation of the pressure plate
    // wolf::Log("Created pressure plate with TriggerComponent at position: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
}
void PlayState::OnTriggerEvent(const TriggerEvent& event) {
    auto* triggerObject = event.m_pTriggerObject;
    if (!triggerObject) return;

    auto* transform = triggerObject->GetComponent<wolf::Transform2D>();
    if (!transform) {
        wolf::Error("Trigger object has no Transform2D component!");
        return;
    }

    glm::vec2 trapPosition = transform->GetGlobalPosition();

    switch (event.m_trapType) {
        case TrapType::SPIKE_TRAP:
            CreateSpikeTrap(trapPosition);
            break;
        case TrapType::BOULDER_TRAP:
            CreateBoulderTrap(trapPosition);
            break;
        default:
            wolf::Log("No trap to create for this trigger.");
            break;
    }
}

int GetGoldVariant(int tileID) {
    switch (tileID) {
        case Tile::FloorSquare:
        case Tile::FloorSmallSquares:
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
void PlayState::OnCutsceneTriggerEvent(const TriggerEvent& event) {
    if (event.m_triggerType == TriggerType::CUTSCENE_SINGLE) {
        StartCutscene("intro");  // Specify cutscene ID as needed
    }
}

void PlayState::StartCutscene(const std::string& cutsceneID) {
    m_pStateManager->PushState(new CutSceneState(m_pStateManager, m_pGameInstance, "data/cutscenes.yaml", cutsceneID));
}

void PlayState::CreateSpikeTrap(const glm::vec2& position) {
    // Create the spike trap object
    auto& trapObj = m_pGameInstance->GetScene().CreateObject2D();

    // Add spike trap sprite
    if (!trapObj.HasAll<wolf::Sprite2D>()) {
        trapObj.AddComponent<wolf::Sprite2D>("data/textures/spiketrap.png");
    }
    auto* sprite = trapObj.GetComponent<wolf::Sprite2D>();
    sprite->SetOriginToCenterOfTexture();

    // Check if Transform2D component exists, and add if not
    if (!trapObj.HasAll<wolf::Transform2D>()) {
        trapObj.AddComponent<wolf::Transform2D>();
    }
    auto* transform = trapObj.GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);
    transform->SetScale(glm::vec2(3.0f));

    // Add collider component
    if (!trapObj.HasAll<ColliderComponent>()) {
        trapObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
    }
    auto* collider = trapObj.GetComponent<ColliderComponent>();
    collider->AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));

    // Add TrapComponent if not already present
    if (!trapObj.HasAll<TrapComponent>()) {
        trapObj.AddComponent<TrapComponent>(50.0f, 5.0f, m_pColliderManager);
    }

    // // Log the creation of the spike trap
    // wolf::Log("Spike trap created at position: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
}

void PlayState::CreateBoulderTrap(const glm::vec2& position) {
    // Create the boulder object
    auto& boulderObj = m_pGameInstance->GetScene().CreateObject2D();

    // Check if the sprite component exists, and add if not
    if (!boulderObj.HasAll<wolf::Sprite2D>()) {
        boulderObj.AddComponent<wolf::Sprite2D>("data/textures/boulder.png");
    }
    auto* sprite = boulderObj.GetComponent<wolf::Sprite2D>();
    sprite->SetOriginToCenterOfTexture();

    // Check if the Transform2D component exists, and add if not
    if (!boulderObj.HasAll<wolf::Transform2D>()) {
        boulderObj.AddComponent<wolf::Transform2D>();
    }
    auto* transform = boulderObj.GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);
    transform->SetScale(glm::vec2(3.0f));

    // Check if the ColliderComponent exists, and add if not
    if (!boulderObj.HasAll<ColliderComponent>()) {
        boulderObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 1, 0);
    }
    auto* collider = boulderObj.GetComponent<ColliderComponent>();
    collider->AddColliderBox(glm::vec2(64.0f, 64.0f), glm::vec2(-32.0f, 32.0f));

    // Check if the BoulderTrapComponent exists, and add if not
    if (!boulderObj.HasAll<BoulderTrapComponent>()) {
        boulderObj.AddComponent<BoulderTrapComponent>(m_pColliderManager, BoulderDirection::RIGHT, 200.0f, 100.0f);
    }

    // Log the creation of the boulder trap
    // wolf::Log("Boulder trap created at position: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
}