#include "ParticleEditor.h"
#include "ParticleComponent.h"
#include <W_GameObject.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include "W_Logging.h"
#include "W_Transform2D.h"
#include "W_Input.h"

bool editorVisible = false;

void ParticleEditor::Update(float delta)
{
    // Toggle visibility on Right Alt press
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT_ALT))
    {
        editorVisible = !editorVisible;
    }

    // Show the editor if the toggle is on
    if (editorVisible)
    {
        ShowEditor();
    }
}

void ParticleEditor::ShowEditor()
{
    auto& scene = GetGameObject()->GetScene();
    ImGui::Begin("Particle Editor");

    for (auto&& [id, particleComponent] : scene.Each<ParticleComponent>())
    {
        auto* particleComponentPtr = &particleComponent;
        EditorState& state = m_editorStates[particleComponentPtr];
        auto* gameObject = particleComponentPtr->GetGameObject();

        // Unique ID for the collapsible section
        std::string nodeLabel = "Particle Component - GameObject " + std::to_string(gameObject->GetID());
        if (ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Editing GameObject ID: %u", gameObject->GetID());

            // Particle Settings
            ImGui::ColorEdit4("Particle Color", &state.color[0]);
            ImGui::SliderFloat("Size", &state.size, 1.0f, 20.0f);
            ImGui::SliderFloat("Lifetime", &state.lifetime, 0.1f, 5.0f);
            ImGui::SliderFloat2("Velocity", &state.velocity[0], -50.0f, 50.0f);

            int maxParticles = static_cast<int>(state.maxParticles);
            if (ImGui::SliderInt("Max Particles", &maxParticles, 10, 500))
            {
                state.maxParticles = static_cast<size_t>(maxParticles);
                particleComponentPtr->SetMaxParticles(state.maxParticles);
            }

            ImGui::Separator();
            ImGui::Text("Sprite Options");

            // Static Sprite Selection
            ImGui::Checkbox("Use Static Sprite", &state.useStaticSprite);
            ImGui::InputText("Static Sprite Path", state.staticSpritePath, sizeof(state.staticSpritePath));

            // Animated Sprite Selection
            ImGui::Checkbox("Use Animated Sprite", &state.useAnimatedSprite);
            ImGui::InputText("Animated Sprite Path", state.animatedSpritePath, sizeof(state.animatedSpritePath));

            // Load sprite when button is pressed
            if (ImGui::Button("Load Sprite"))
            {
                LoadSpriteForParticles(*particleComponentPtr, state);
            }

            if (ImGui::Button("Emit Particle"))
            {
                // Emit particle with user-selected sprites
                wolf::Sprite2D* staticSprite = state.useStaticSprite ? new wolf::Sprite2D(state.staticSpritePath) : nullptr;
                AnimatedSprite2D* animatedSprite = state.useAnimatedSprite ? new AnimatedSprite2D(state.animatedSpritePath) : nullptr;

                particleComponentPtr->Emit(
                    gameObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(),
                    state.velocity,
                    state.color,
                    state.size,
                    state.lifetime,
                    state.useAnimatedSprite,
                    animatedSprite,
                    staticSprite
                );
            }

            char buffer[256];
            strncpy(buffer, state.configFilePath.c_str(), sizeof(buffer));
            if (ImGui::InputText("Config File Path", buffer, sizeof(buffer)))
            {
                state.configFilePath = buffer;
            }
            if (ImGui::Button("Save Config"))
            {
                SaveConfigToYAML(*particleComponentPtr, state.configFilePath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Config"))
            {
                LoadConfigFromYAML(*particleComponentPtr, state.configFilePath);
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void ParticleEditor::LoadSpriteForParticles(ParticleComponent& particleComponent, EditorState& state)
{
    if (state.useStaticSprite)
    {
        wolf::Log("Loading Static Sprite: ", state.staticSpritePath);
        // Create static sprite and assign it to future particles
        wolf::Sprite2D* sprite = new wolf::Sprite2D(state.staticSpritePath);
        for (auto& particle : particleComponent.GetParticles())
        {
            if (!particle.m_active)
                particle.m_staticSprite = sprite;
        }
    }

    if (state.useAnimatedSprite)
    {
        wolf::Log("Loading Animated Sprite: ", state.animatedSpritePath);
        // Create animated sprite and assign it to future particles
        AnimatedSprite2D* animSprite = new AnimatedSprite2D(state.animatedSpritePath);
        for (auto& particle : particleComponent.GetParticles())
        {
            if (!particle.m_active)
                particle.m_animatedSprite = animSprite;
        }
    }
}


void ParticleEditor::ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state)
{
    particleComponent.SetMaxParticles(state.maxParticles);
}

void ParticleEditor::SaveConfigToYAML(ParticleComponent& particleComponent, const std::string& filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        wolf::Error("Failed to open file for saving: ", filename.c_str());
        return;
    }

    EditorState& state = m_editorStates[&particleComponent];

    file << "particle_config:\n";
    file << "  color: [" << state.color.r << ", " << state.color.g << ", " << state.color.b << ", " << state.color.a << "]\n";
    file << "  size: " << state.size << "\n";
    file << "  lifetime: " << state.lifetime << "\n";
    file << "  velocity: { x: " << state.velocity.x << ", y: " << state.velocity.y << " }\n";
    file << "  max_particles: " << state.maxParticles << "\n";

    wolf::Log("Particle configuration saved to ", filename.c_str());
}

void ParticleEditor::LoadConfigFromYAML(ParticleComponent& particleComponent, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        wolf::Error("Failed to load config file: ", filename.c_str());
        return;
    }

    YAML::Node config = YAML::LoadFile(filename);
    EditorState& state = m_editorStates[&particleComponent];

    YAML::Node particleConfig = config["particle_config"];
    if (particleConfig)
    {
        if (particleConfig["color"])
        {
            auto color = particleConfig["color"].as<std::vector<float>>();
            if (color.size() == 4)
                state.color = glm::vec4(color[0], color[1], color[2], color[3]);
        }

        if (particleConfig["size"])
            state.size = particleConfig["size"].as<float>();

        if (particleConfig["lifetime"])
            state.lifetime = particleConfig["lifetime"].as<float>();

        if (particleConfig["velocity"])
        {
            state.velocity.x = particleConfig["velocity"]["x"].as<float>();
            state.velocity.y = particleConfig["velocity"]["y"].as<float>();
        }

        if (particleConfig["max_particles"])
        {
            state.maxParticles = particleConfig["max_particles"].as<size_t>();
            particleComponent.SetMaxParticles(state.maxParticles);
        }

        wolf::Log("Particle configuration loaded from ", filename.c_str());
    }
    else
    {
        wolf::Error("Invalid config format in file: ", filename.c_str());
    }
}
