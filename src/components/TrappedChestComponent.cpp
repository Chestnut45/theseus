//-----------------------------------------------------------------------------
// File: TrappedChestComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Trap component for chests
//-----------------------------------------------------------------------------
#include "TrappedChestComponent.h"

#include "AnimatedSprite2D.h"
#include "HealthComponent.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include "MinitaurController.h"
#include "PlayerController.h"
#include "TimedDestroyerComponent.h"
#include "VelocityComponent.h"

#include "EnemyDataLoader.h"
#include "GorgonBuilder.h"
#include "HarpyBuilder.h"
#include "MinitaurBuilder.h"

std::vector<std::string> TrappedChestComponent::s_vTaunts;
std::vector<ImVec2> TrappedChestComponent::s_vTauntTextSizes;
TrappedChestComponent::TrappedChestComponent(TrapType p_trap_event)
{
    if(s_iComponentCount == 0)
    {
        InitTaunts();
    }
    s_iComponentCount++;
    this->m_iID = s_iComponentCount;
    this->m_trapEvent = p_trap_event;
    this->m_bIsOpen = false;
    this->m_iTauntIndex = m_RNG.NextInt(0, TrappedChestComponent::s_vTaunts.size() - 1);
}

TrappedChestComponent::~TrappedChestComponent()
{

}

bool TrappedChestComponent::IsOpen()
{
    return this->m_bIsOpen;
}

void TrappedChestComponent::Update(float p_delta)
{   
    // If trap is triggered, count down
    if(this->m_bIsOpen == true)
    {
        m_fSelfDestructTimer -= p_delta;

        this->DisplayTaunt();

        // If countdown is over
        if(m_fSelfDestructTimer <= 0.0f)
        {
            
            wolf::GameObject* gameObj = GetGameObject();
            wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
            wolf::Scene* scene = &gameObj->GetScene();

            switch (m_trapEvent)
            {
            case TrapType::TRANSFORM_GORGON:
                this->SpawnGorgon();
                break;
            
            
            case TrapType::TRANSFORM_HARPY:
                this->SpawnHarpy();
                break;

            
            case TrapType::TRANSFORM_MINITAUR:
                this->SpawnMinitaur();
                break;

            
            case TrapType::EXPLODE:
            {
                this->Explode();
                break;
            }

            default:
                break;
            }

            // Destroy chest
            scene->DeleteObject(gameObj->GetID());
        }
    }
}

void TrappedChestComponent::OpenTrappedChest()
{
    this->m_bIsOpen = true;
}

void TrappedChestComponent::DisplayTaunt()
{
    wolf::GameObject* gameObj = this->GetGameObject();
    wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
    wolf::Scene* scene = &gameObj->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    glm::vec2 cameraPos = camera->GetPosition();
    glm::vec2 viewSize = camera->GetViewSize();
    glm::vec2 worldpos = gameObjTransform->GetGlobalPosition() + glm::vec2(0.0f, 30.0f) * gameObjTransform->GetGlobalScale();

    glm::vec2 screenpos;
    screenpos.x = (worldpos.x - (cameraPos.x - viewSize.x * 0.5f));
    screenpos.y = (worldpos.y - (cameraPos.y - viewSize.y * 0.5f)) * (-1) + viewSize.y;
    screenpos += glm::vec2(-TrappedChestComponent::WINDOW_SIZE_HALF.x, TrappedChestComponent::WINDOW_SIZE_HALF.y);

    std::string windowName = "TrappedChest" + std::to_string(m_iID);
    std::string taunt = TrappedChestComponent::s_vTaunts.at(m_iTauntIndex);
    
    // Setup
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;
    ImGui::SetNextWindowPos({screenpos.x, screenpos.y});
    ImGui::SetNextWindowSize(TrappedChestComponent::WINDOW_SIZE);
    ImGui::Begin(windowName.c_str(), nullptr, flags);
    ImGui::SetCursorPosX((TrappedChestComponent::WINDOW_SIZE.x - s_vTauntTextSizes.at(m_iTauntIndex).x) * 0.5f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", taunt.c_str());

    // End rendering
    ImGui::End();
}

void TrappedChestComponent::Explode()
{
    wolf::GameObject* gameObj = GetGameObject();
    wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
    wolf::Scene* scene = &gameObj->GetScene();

    // Spawn explosion
    wolf::GameObject* explosion = &scene->CreateObject2D();

    explosion->GetComponent<wolf::Transform2D>()->SetPosition(gameObjTransform->GetGlobalPosition());
    explosion->GetComponent<wolf::Transform2D>()->SetScale(gameObjTransform->GetGlobalScale() * 3.0f);

    wolf::Sprite2D* explosionSprite = &explosion->AddComponent<wolf::Sprite2D>("data/textures/Fireball.png");
    explosionSprite->SetOriginToCenterOfTexture();
    
    explosion->AddComponent<TimedDestroyerComponent>(0.5f);

    // Get player
    PlayerController* playerController;
    for(auto&& [_, player_controller] : scene->Each<PlayerController>())
    {
        playerController = &player_controller;
        break;
    }
    wolf::GameObject* player = playerController->GetGameObject();
    wolf::Transform2D* playerTransform = player->GetComponent<wolf::Transform2D>();
    glm::vec2 line = playerTransform->GetGlobalPosition() - gameObjTransform->GetGlobalPosition();
    float distance = glm::length(line);

    float blastRadius = 100.0f * gameObjTransform->GetGlobalScale().x;
    // If player is in blast radius, deal damage
    if(distance <= blastRadius)
    {
        float blastDamage = blastRadius - distance;
        blastDamage = blastDamage > 0.0f ? blastDamage: 0.0f;
        blastDamage *= 1.5f;

        float knockbackForce = blastRadius - distance;
        knockbackForce = knockbackForce > 0.0f ? knockbackForce : 0.0f;
        knockbackForce *= 10.0f;

        player->GetComponent<HealthComponent>()->Damage(blastDamage);
        player->GetComponent<VelocityComponent>()->ApplyKnockback(line, knockbackForce);
    }
    // TODO - Apply line-of-sight check to explosion to account for walls
    // TODO - Apply damage to all nearby enemies
}

// Spawns Gorgon
void TrappedChestComponent::SpawnGorgon()
{
    wolf::GameObject* gameObj = GetGameObject();
    wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
    wolf::Scene* scene = &gameObj->GetScene();

    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    GorgonBuilder gorgonBuilder(gameObj->GetScene());

    glm::vec2 position = gameObjTransform->GetGlobalPosition();

    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    auto& gorgon = gorgonBuilder.BuildGorgon(gorgonData, position);
    
    // Set gorgon scale
    auto* transform = gorgon.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(gameObjTransform->GetGlobalScale());
    }
}

// Spawns Harpy
void TrappedChestComponent::SpawnHarpy()
{
    wolf::GameObject* gameObj = GetGameObject();
    wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
    wolf::Scene* scene = &gameObj->GetScene();

    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    HarpyBuilder harpyBuilder(gameObj->GetScene());
    glm::vec2 position = gameObjTransform->GetGlobalPosition();

    EnemyData harpyData = loader.LoadEnemyData("harpy");
    auto& harpy = harpyBuilder.BuildHarpy(harpyData, position);
    
    // Set harpy scale
    auto* transform = harpy.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(gameObjTransform->GetGlobalScale());
    }
}

// Spawns Minitaur
void TrappedChestComponent::SpawnMinitaur()
{
    wolf::GameObject* gameObj = GetGameObject();
    wolf::Transform2D* gameObjTransform = gameObj->GetComponent<wolf::Transform2D>();
    wolf::Scene* scene = &gameObj->GetScene();

    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");

    MinitaurBuilder minitaurBuilder(gameObj->GetScene());

    glm::vec2 position = gameObjTransform->GetGlobalPosition(); 

    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    auto& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, position);
    
    // Set minitaur scale
    auto* transform = minitaur.GetComponent<wolf::Transform2D>();
    if (transform)
    {
        transform->SetScale(glm::vec2(3.0f));
    }

    auto* statusComponent = minitaur.GetComponent<StatusComponent>();
}

void TrappedChestComponent::InitTaunts()
{
    s_vTaunts =
    {
        "From Eris With Love <3",
        "Blessing From Eris :D",
        "'Hope ya like it! :)))' - Eris"
    };

    s_vTauntTextSizes = 
    {
        ImGui::CalcTextSize(TrappedChestComponent::s_vTaunts.at(0).c_str()),
        ImGui::CalcTextSize(TrappedChestComponent::s_vTaunts.at(1).c_str()),
        ImGui::CalcTextSize(TrappedChestComponent::s_vTaunts.at(2).c_str())
    };
}