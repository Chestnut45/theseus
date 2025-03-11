#include "ParticleEditor.h"
#include "ParticleComponent.h"
#include <W_GameObject.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include "W_Logging.h"
#include "W_Transform2D.h"
#include "W_Input.h"
#include <filesystem>

void ParticleEditor::Update(float delta)
{
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT_ALT))
    {
        m_editorVisible = !m_editorVisible;
    }

    if (m_editorVisible)
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

        std::string nodeLabel = "Particle Component - GameObject " + std::to_string(gameObject->GetID());
        if (ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Editing GameObject ID: %u", gameObject->GetID());
            ImGui::Text("Active Particles: %d/%d", CountActiveParticles(*particleComponentPtr), 
                                                  static_cast<int>(particleComponentPtr->GetMaxParticles()));

            // Basic particle properties
            if (ImGui::CollapsingHeader("Particle Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit4("Particle Color", &state.color[0]);
                ImGui::SliderFloat("Size", &state.size, 1.0f, 50.0f);
                ImGui::SliderFloat("Lifetime", &state.lifetime, 0.1f, 10.0f);
                ImGui::SliderFloat2("Velocity", &state.velocity[0], -100.0f, 100.0f);
                
                ImGui::SeparatorText("System Properties");
                int maxParticles = static_cast<int>(state.maxParticles);
                if (ImGui::SliderInt("Max Particles", &maxParticles, 10, 1000))
                {
                    state.maxParticles = static_cast<size_t>(maxParticles);
                    particleComponentPtr->SetMaxParticles(state.maxParticles);
                }
                
                ImGui::SliderFloat("Emission Rate", &state.emissionRate, 0.0f, 50.0f, "%.1f particles/sec");
                ImGui::Checkbox("Continuous Emission", &state.continuousEmission);
            }

            // Texture options
            if (ImGui::CollapsingHeader("Texture Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Use Texture", &state.useTexture);
                
                if (state.useTexture)
                {
                    // Input for texture path without automatic loading
                    ImGui::InputText("Texture Path", state.texturePath, sizeof(state.texturePath));
                    
                    // Browse button - simulates a file dialog
                    if (ImGui::Button("Browse..."))
                    {
                        // Simulate file browser with common texture paths
                        ImGui::OpenPopup("Texture Browser");
                    }
                    
                    // Texture browser popup
                    if (ImGui::BeginPopup("Texture Browser"))
                    {
                        ImGui::Text("Select a texture:");
                        ImGui::Separator();
                        
                        // List of sample textures just for demo and presentation - replace with actual texture paths
                        static const char* texturePaths[] = {
                            "data/textures/particle.png",
                            "data/textures/smoke.png",
                            "data/textures/fire.png",
                            "data/textures/spark.png",
                            "data/textures/Arrow.png",
                            "data/textures/circle.png",
                            "data/textures/star.png"
                        };
                        
                        for (const char* path : texturePaths)
                        {
                            if (ImGui::Selectable(path))
                            {
                                strncpy(state.texturePath, path, sizeof(state.texturePath));
                                
                                // Clear previous texture
                                if (state.previewTexture)
                                {
                                    // Note: Don't destroy here, just clear the pointer
                                    // The texture manager should handle lifetime
                                    state.previewTexture = nullptr;
                                }
                                
                                // Load the texture immediately
                                state.previewTexture = wolf::TextureManager::CreateTexture(state.texturePath);
                                if (state.previewTexture)
                                {
                                    wolf::Log("Loaded texture: ", path);
                                    state.textureLoaded = true;
                                }
                                else
                                {
                                    wolf::Error("Failed to load texture: ", path);
                                    state.textureLoaded = false;
                                }
                            }
                        }
                        
                        ImGui::EndPopup();
                    }
                    
                    // Load texture button
                    if (ImGui::Button("Load Texture"))
                    {
                        // Clear previous texture
                        if (state.previewTexture)
                        {
                            // Just clear the pointer, don't destroy
                            state.previewTexture = nullptr;
                        }
                        
                        // Try to load the texture
                        state.previewTexture = wolf::TextureManager::CreateTexture(state.texturePath);
                        if (state.previewTexture)
                        {
                            wolf::Log("Loaded texture: ", state.texturePath);
                            state.textureLoaded = true;
                        }
                        else
                        {
                            wolf::Error("Failed to load texture: ", state.texturePath);
                            state.textureLoaded = false;
                        }
                    }
                    
                    // Preview texture if available
                    if (state.previewTexture && state.textureLoaded)
                    {
                        ImGui::Text("Texture Preview:");
                        ImGui::Image((void*)(intptr_t)state.previewTexture->GetID(), ImVec2(64, 64));
                    }
                    else if (!state.textureLoaded)
                    {
                        ImGui::TextColored(ImVec4(1,0,0,1), "Texture not found or invalid");
                    }
                }
                else
                {
                    // If textures are disabled, clear any loaded texture
                    if (state.previewTexture)
                    {
                        state.previewTexture = nullptr;
                        state.textureLoaded = false;
                    }
                }
            }

            // Emission controls
            if (ImGui::CollapsingHeader("Emission Controls", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("Emit Single Particle"))
                {
                    EmitParticle(*particleComponentPtr, state, 
                                gameObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Emit Burst"))
                {
                    EmitParticleBurst(*particleComponentPtr, state, 
                                    gameObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(),
                                    state.burstCount);
                }
                
                ImGui::SliderInt("Burst Count", &state.burstCount, 1, 50);
                
                if (state.continuousEmission)
                {
                    state.emissionTimer += ImGui::GetIO().DeltaTime;
                    float emissionInterval = 1.0f / std::max(0.1f, state.emissionRate);
                    
                    while (state.emissionTimer >= emissionInterval)
                    {
                        EmitParticle(*particleComponentPtr, state, 
                                    gameObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
                        state.emissionTimer -= emissionInterval;
                    }
                }
                else
                {
                    state.emissionTimer = 0.0f;
                }
            }

            // Config management
            if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
            {
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
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

int ParticleEditor::CountActiveParticles(ParticleComponent& particleComponent)
{
    int count = 0;
    for (const auto& particle : particleComponent.GetParticles())
    {
        if (particle.m_active)
        {
            count++;
        }
    }
    return count;
}

void ParticleEditor::EmitParticle(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position)
{
    wolf::Texture* texture = nullptr;
    if (state.useTexture && state.textureLoaded)
    {
        // Use the preloaded texture instead of loading it again
        texture = state.previewTexture;
        
        // Debug texture information
        if (texture)
        {
            // wolf::Log("Emitting particle with texture. ID: ", texture->GetID());
            
            // Check if texture is valid
            GLuint texID = texture->GetID();
            GLint width = 0, height = 0;
            
            // Temporarily bind to check parameters
            glBindTexture(GL_TEXTURE_2D, texID);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            // wolf::Log("Texture dimensions: ", width, "x", height);
            
            if (width == 0 || height == 0)
            {
                wolf::Error("Texture appears to be invalid - zero dimensions!");
            }
        }
        else
        {
            wolf::Error("Texture marked as loaded but is nullptr!");
        }
    }
    else if (state.useTexture)
    {
        wolf::Error("Texture use enabled but not marked as loaded!");
    }
    
    particleComponent.Emit(
        position,
        state.velocity,
        state.color,
        state.size,
        state.lifetime,
        texture
    );
    
    // Verify the particle was created with the texture
    int count = 0;
    for (const auto& particle : particleComponent.GetParticles())
    {
        if (particle.m_active && particle.m_texture == texture)
        {
            count++;
        }
    }
    
    // wolf::Log("Emitted particle. Active particles with this texture: ", count);
}

void ParticleEditor::EmitParticleBurst(ParticleComponent& particleComponent, EditorState& state, 
                                      const glm::vec2& position, int count)
{
    wolf::Texture* texture = nullptr;
    if (state.useTexture && state.textureLoaded)
    {
        // Use the preloaded texture instead of loading it again
        texture = state.previewTexture;
        
        // Debug texture information
        // wolf::Log("Emitting burst with texture ID: ", texture ? texture->GetID() : 0);
    }
    
    // Calculate how many particles to emit in this burst
    int numToEmit = std::min(count, static_cast<int>(particleComponent.GetMaxParticles()));
    
    // Log burst information
    // wolf::Log("Emitting burst of ", numToEmit, " particles");
    
    for (int i = 0; i < numToEmit; i++)
    {
        // Create some variation for the burst
        float angleVariation = static_cast<float>(i) / numToEmit * 360.0f;
        float radians = glm::radians(angleVariation);
        
        glm::vec2 variedVelocity;
        variedVelocity.x = state.velocity.x * cosf(radians) - state.velocity.y * sinf(radians);
        variedVelocity.y = state.velocity.x * sinf(radians) + state.velocity.y * cosf(radians);
        
        // Modify size to be more visible
        float sizeVariation = std::max(1.0f, state.size * (0.8f + 0.4f * static_cast<float>(rand()) / RAND_MAX));
        
        // Add more variation to lifetime to prevent all particles disappearing at once
        float lifetimeVariation = state.lifetime * (0.5f + static_cast<float>(rand()) / RAND_MAX);
        
        // Add position variation to spread particles out
        glm::vec2 posVariation(
            position.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f,
            position.y + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f
        );
        
        particleComponent.Emit(
            posVariation,
            variedVelocity,
            state.color,
            sizeVariation,
            lifetimeVariation,
            texture
        );
    }
    
    // Log completion
    // wolf::Log("Burst emission complete");
}

void ParticleEditor::ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state)
{
    particleComponent.SetMaxParticles(state.maxParticles);
}

void ParticleEditor::SaveConfigToYAML(ParticleComponent& particleComponent, const std::string& filename)
{
    try {
        std::filesystem::path dir = std::filesystem::path(filename).parent_path();
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        std::ofstream file(filename);
        if (!file.is_open())
        {
            wolf::Error("Failed to open file for saving: ", filename.c_str());
            return;
        }

        EditorState& state = m_editorStates[&particleComponent];

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "particle_config" << YAML::Value;
        out << YAML::BeginMap;
        
        // Basic properties
        out << YAML::Key << "color" << YAML::Value << YAML::Flow << YAML::BeginSeq 
            << state.color.r << state.color.g << state.color.b << state.color.a << YAML::EndSeq;
        out << YAML::Key << "size" << YAML::Value << state.size;
        out << YAML::Key << "lifetime" << YAML::Value << state.lifetime;
        
        // Velocity
        out << YAML::Key << "velocity" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "x" << YAML::Value << state.velocity.x;
        out << YAML::Key << "y" << YAML::Value << state.velocity.y;
        out << YAML::EndMap;
        
        // System properties
        out << YAML::Key << "max_particles" << YAML::Value << state.maxParticles;
        out << YAML::Key << "emission_rate" << YAML::Value << state.emissionRate;
        out << YAML::Key << "continuous_emission" << YAML::Value << state.continuousEmission;
        out << YAML::Key << "burst_count" << YAML::Value << state.burstCount;
        
        // Texture properties
        out << YAML::Key << "use_texture" << YAML::Value << state.useTexture;
        out << YAML::Key << "texture_path" << YAML::Value << state.texturePath;
        
        out << YAML::EndMap;
        out << YAML::EndMap;
        
        file << out.c_str();
        file.close();

        wolf::Log("Particle configuration saved to ", filename.c_str());
    }
    catch (const std::exception& e) {
        wolf::Error("Exception while saving particle config: ", e.what());
    }
}

void ParticleEditor::LoadConfigFromYAML(ParticleComponent& particleComponent, const std::string& filename)
{
    try {
        if (!std::filesystem::exists(filename)) {
            wolf::Error("Config file does not exist: ", filename.c_str());
            return;
        }
        
        YAML::Node config = YAML::LoadFile(filename);
        EditorState& state = m_editorStates[&particleComponent];

        YAML::Node particleConfig = config["particle_config"];
        if (particleConfig)
        {
            // Basic properties
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

            // Velocity
            if (particleConfig["velocity"])
            {
                state.velocity.x = particleConfig["velocity"]["x"].as<float>();
                state.velocity.y = particleConfig["velocity"]["y"].as<float>();
            }

            // System properties
            if (particleConfig["max_particles"])
            {
                state.maxParticles = particleConfig["max_particles"].as<size_t>();
                particleComponent.SetMaxParticles(state.maxParticles);
            }
            
            if (particleConfig["emission_rate"])
                state.emissionRate = particleConfig["emission_rate"].as<float>();
                
            if (particleConfig["continuous_emission"])
                state.continuousEmission = particleConfig["continuous_emission"].as<bool>();
                
            if (particleConfig["burst_count"])
                state.burstCount = particleConfig["burst_count"].as<int>();

            // Texture properties
            if (particleConfig["use_texture"])
                state.useTexture = particleConfig["use_texture"].as<bool>();

            if (particleConfig["texture_path"])
                strncpy(state.texturePath, particleConfig["texture_path"].as<std::string>().c_str(), sizeof(state.texturePath));

            wolf::Log("Particle configuration loaded from ", filename.c_str());
        }
        else
        {
            wolf::Error("Invalid config format in file: ", filename.c_str());
        }
    }
    catch (const std::exception& e) {
        wolf::Error("Exception while loading particle config: ", e.what());
    }
}