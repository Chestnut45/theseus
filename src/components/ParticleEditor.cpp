#include "ParticleEditor.h"
#include "ParticleComponent.h"
#include "ParticleModifier.h"
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

    // if (m_editorVisible)
    // {
    //     ShowEditor();
    // }
}

void ParticleEditor::ShowEditor()
{
    auto& scene = GetGameObject()->GetScene();
    ImGui::Begin("Enhanced Particle Editor");

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
                
                // Enhanced particle properties
                ImGui::SliderFloat("Initial Rotation", &state.rotation, 0.0f, 360.0f);
                ImGui::SliderFloat("Angular Velocity", &state.angularVelocity, -360.0f, 360.0f, "%.1f deg/s");
                
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
            
            // Particle Modifiers
            if (ImGui::CollapsingHeader("Particle Modifiers", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ShowModifiersPanel(*particleComponentPtr, state);
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
                ImGui::SliderFloat("Burst Spread", &state.burstSpread, 0.0f, 360.0f, "%.1f degrees");
                
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

// Update this method to use the new EmitBurst method when available
void ParticleEditor::EmitParticleBurst(ParticleComponent& particleComponent, EditorState& state, 
                                      const glm::vec2& position, int count)
{
    wolf::Texture* texture = nullptr;
    if (state.useTexture && state.textureLoaded)
    {
        texture = state.previewTexture;
    }
    
    // Use the new EmitBurst method if available
    particleComponent.EmitBurst(
        position,
        state.velocity,
        state.color,
        state.size,
        state.lifetime,
        count,
        state.burstSpread,
        texture
    );
}

// Update this method to include rotation parameters
void ParticleEditor::EmitParticle(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position)
{
    wolf::Texture* texture = nullptr;
    if (state.useTexture && state.textureLoaded)
    {
        texture = state.previewTexture;
    }
    
    particleComponent.Emit(
        position,
        state.velocity,
        state.color,
        state.size,
        state.lifetime,
        texture,
        state.rotation,
        state.angularVelocity
    );
}

// Add the modifiers panel UI
void ParticleEditor::ShowModifiersPanel(ParticleComponent& particleComponent, EditorState& state)
{
    ImGui::Text("Behavior Modifiers:");
    
    // Emission Shape
    if (ImGui::Checkbox("Emission Shape", &state.showEmissionShapeEditor))
    {
        if (state.showEmissionShapeEditor)
        {
            // Create the modifier if it doesn't exist
            GetEmissionShapeModifier(particleComponent, true);
        }
        else
        {
            // Remove the modifier if it exists
            particleComponent.RemoveModifier("Emission Shape");
        }
    }
    
    if (state.showEmissionShapeEditor)
    {
        ImGui::Indent();
        ShowEmissionShapeEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Gravity
    if (ImGui::Checkbox("Gravity", &state.showGravityEditor))
    {
        if (state.showGravityEditor)
        {
            GetGravityModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Gravity");
        }
    }
    
    if (state.showGravityEditor)
    {
        ImGui::Indent();
        ShowGravityEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Drag
    if (ImGui::Checkbox("Drag", &state.showDragEditor))
    {
        if (state.showDragEditor)
        {
            GetDragModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Drag");
        }
    }
    
    if (state.showDragEditor)
    {
        ImGui::Indent();
        ShowDragEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Vortex
    if (ImGui::Checkbox("Vortex", &state.showVortexEditor))
    {
        if (state.showVortexEditor)
        {
            GetVortexModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Vortex");
        }
    }
    
    if (state.showVortexEditor)
    {
        ImGui::Indent();
        ShowVortexEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Attractor/Repeller
    if (ImGui::Checkbox("Attractor", &state.showAttractorEditor))
    {
        if (state.showAttractorEditor)
        {
            GetAttractorModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Attractor");
        }
    }
    
    if (state.showAttractorEditor)
    {
        ImGui::Indent();
        ShowAttractorEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    ImGui::Separator();
    ImGui::Text("Appearance Modifiers:");
    
    // Size Over Lifetime
    if (ImGui::Checkbox("Size Over Lifetime", &state.showSizeOverLifetimeEditor))
    {
        if (state.showSizeOverLifetimeEditor)
        {
            GetSizeOverLifetimeModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Size Over Lifetime");
        }
    }
    
    if (state.showSizeOverLifetimeEditor)
    {
        ImGui::Indent();
        ShowSizeOverLifetimeEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Color Over Lifetime
    if (ImGui::Checkbox("Color Over Lifetime", &state.showColorOverLifetimeEditor))
    {
        if (state.showColorOverLifetimeEditor)
        {
            GetColorOverLifetimeModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Color Over Lifetime");
        }
    }
    
    if (state.showColorOverLifetimeEditor)
    {
        ImGui::Indent();
        ShowColorOverLifetimeEditor(particleComponent, state);
        ImGui::Unindent();
    }
    
    // Rotation
    if (ImGui::Checkbox("Rotation", &state.showRotationEditor))
    {
        if (state.showRotationEditor)
        {
            GetRotationModifier(particleComponent, true);
        }
        else
        {
            particleComponent.RemoveModifier("Rotation");
        }
    }
    
    if (state.showRotationEditor)
    {
        ImGui::Indent();
        ShowRotationEditor(particleComponent, state);
        ImGui::Unindent();
    }
}

// Implement the editor interfaces for each modifier
void ParticleEditor::ShowEmissionShapeEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetEmissionShapeModifier(particleComponent);
    if (!modifier) return;
    
    // Shape type selection
    static const char* shapeTypes[] = { "Point", "Line", "Circle", "Rectangle", "Ring" };
    int shapeType = static_cast<int>(modifier->GetShapeType());
    if (ImGui::Combo("Shape Type", &shapeType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
    {
        modifier->SetShapeType(static_cast<EmissionShapeModifier::ShapeType>(shapeType));
    }
    
    // Shape-specific parameters
    switch (shapeType)
    {
        case EmissionShapeModifier::LINE:
        {
            float width = 10.0f; // We don't have getter for this, so use placeholder
            if (ImGui::SliderFloat("Width", &width, 1.0f, 100.0f))
            {
                modifier->SetSize(width, 0.0f);
            }
            break;
        }
        
        case EmissionShapeModifier::CIRCLE:
        {
            float radius = 20.0f; // We don't have getter for this, so use placeholder
            if (ImGui::SliderFloat("Radius", &radius, 1.0f, 100.0f))
            {
                modifier->SetRadius(radius);
            }
            
            bool emitFromEdge = modifier->GetEmitFromEdge();
            if (ImGui::Checkbox("Emit From Edge Only", &emitFromEdge))
            {
                modifier->SetEmitFromEdge(emitFromEdge);
            }
            break;
        }
        
        case EmissionShapeModifier::RECTANGLE:
        {
            float width = 10.0f;
            float height = 10.0f;
            // We don't have getters for these, so use placeholders
            if (ImGui::SliderFloat("Width", &width, 1.0f, 100.0f) ||
                ImGui::SliderFloat("Height", &height, 1.0f, 100.0f))
            {
                modifier->SetSize(width, height);
            }
            
            bool emitFromEdge = modifier->GetEmitFromEdge();
            if (ImGui::Checkbox("Emit From Edge Only", &emitFromEdge))
            {
                modifier->SetEmitFromEdge(emitFromEdge);
            }
            break;
        }
        
        case EmissionShapeModifier::RING:
        {
            float outerRadius = 20.0f;
            float innerRadius = 10.0f;
            // We don't have getters for these, so use placeholders
            if (ImGui::SliderFloat("Outer Radius", &outerRadius, 1.0f, 100.0f))
            {
                modifier->SetRadius(outerRadius);
            }
            
            if (ImGui::SliderFloat("Inner Radius", &innerRadius, 1.0f, outerRadius - 1.0f))
            {
                modifier->SetInnerRadius(innerRadius);
            }
            break;
        }
    }
    
    // Direction controls
    bool randomDirection = modifier->GetRandomDirection();
    if (ImGui::Checkbox("Random Direction", &randomDirection))
    {
        modifier->SetRandomDirection(randomDirection);
    }
    
    if (!randomDirection)
    {
        float directionAngle = modifier->GetDirectionAngle();
        if (ImGui::SliderFloat("Direction Angle", &directionAngle, 0.0f, 360.0f, "%.1f degrees"))
        {
            modifier->SetDirectionAngle(directionAngle);
        }
        
        float spreadAngle = modifier->GetSpreadAngle();
        if (ImGui::SliderFloat("Spread Angle", &spreadAngle, 0.0f, 360.0f, "%.1f degrees"))
        {
            modifier->SetSpreadAngle(spreadAngle);
        }
    }
}

void ParticleEditor::ShowGravityEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetGravityModifier(particleComponent);
    if (!modifier) return;
    
    glm::vec2 gravity = modifier->GetGravity();
    if (ImGui::SliderFloat2("Gravity Vector", &gravity[0], -100.0f, 100.0f))
    {
        modifier->SetGravity(gravity);
    }
    
    float strength = modifier->GetStrength();
    if (ImGui::SliderFloat("Strength", &strength, 0.0f, 5.0f))
    {
        modifier->SetStrength(strength);
    }
}

void ParticleEditor::ShowDragEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetDragModifier(particleComponent);
    if (!modifier) return;
    
    float dragCoefficient = modifier->GetDragCoefficient();
    if (ImGui::SliderFloat("Drag Coefficient", &dragCoefficient, 0.0f, 2.0f))
    {
        modifier->SetDragCoefficient(dragCoefficient);
    }
}

void ParticleEditor::ShowVortexEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetVortexModifier(particleComponent);
    if (!modifier) return;
    
    glm::vec2 centerOffset = modifier->GetCenterOffset();
    if (ImGui::SliderFloat2("Center Offset", &centerOffset[0], -100.0f, 100.0f))
    {
        modifier->SetCenterOffset(centerOffset);
    }
    
    float rotationSpeed = modifier->GetRotationSpeed();
    if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, -360.0f, 360.0f, "%.1f deg/s"))
    {
        modifier->SetRotationSpeed(rotationSpeed);
    }
    
    float strength = modifier->GetStrength();
    if (ImGui::SliderFloat("Strength", &strength, 0.0f, 5.0f))
    {
        modifier->SetStrength(strength);
    }
    
    float attractionStrength = modifier->GetAttractionStrength();
    if (ImGui::SliderFloat("Attraction", &attractionStrength, -5.0f, 5.0f))
    {
        modifier->SetAttractionStrength(attractionStrength);
    }
}

void ParticleEditor::ShowAttractorEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetAttractorModifier(particleComponent);
    if (!modifier) return;
    
    glm::vec2 attractorOffset = modifier->GetAttractorOffset();
    if (ImGui::SliderFloat2("Attractor Offset", &attractorOffset[0], -100.0f, 100.0f))
    {
        modifier->SetAttractorOffset(attractorOffset);
    }
    
    float strength = modifier->GetStrength();
    if (ImGui::SliderFloat("Strength", &strength, 0.0f, 10.0f))
    {
        modifier->SetStrength(strength);
    }
    
    float falloffRadius = modifier->GetFalloffRadius();
    if (ImGui::SliderFloat("Falloff Radius", &falloffRadius, 0.0f, 200.0f))
    {
        modifier->SetFalloffRadius(falloffRadius);
    }
    
    bool isRepeller = modifier->GetIsRepeller();
    if (ImGui::Checkbox("Is Repeller", &isRepeller))
    {
        modifier->SetIsRepeller(isRepeller);
    }
}

void ParticleEditor::ShowSizeOverLifetimeEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetSizeOverLifetimeModifier(particleComponent);
    if (!modifier) return;
    
    float startScale = modifier->GetStartScale();
    float endScale = modifier->GetEndScale();
    
    if (ImGui::SliderFloat("Start Scale", &startScale, 0.1f, 5.0f) ||
        ImGui::SliderFloat("End Scale", &endScale, 0.1f, 5.0f))
    {
        modifier->SetScales(startScale, endScale);
    }
    
    static const char* curveTypes[] = { "Linear", "Ease In", "Ease Out", "Ease In-Out" };
    int curveType = static_cast<int>(modifier->GetCurveType());
    
    if (ImGui::Combo("Curve Type", &curveType, curveTypes, IM_ARRAYSIZE(curveTypes)))
    {
        modifier->SetCurveType(static_cast<SizeOverLifetimeModifier::CurveType>(curveType));
    }
}

void ParticleEditor::ShowColorOverLifetimeEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetColorOverLifetimeModifier(particleComponent);
    if (!modifier) return;
    
    glm::vec4 startColor = modifier->GetStartColor();
    glm::vec4 endColor = modifier->GetEndColor();
    
    if (ImGui::ColorEdit4("Start Color", &startColor[0]) ||
        ImGui::ColorEdit4("End Color", &endColor[0]))
    {
        modifier->SetColors(startColor, endColor);
    }
}

void ParticleEditor::ShowRotationEditor(ParticleComponent& particleComponent, EditorState& state)
{
    auto modifier = GetRotationModifier(particleComponent);
    if (!modifier) return;
    
    float rotationSpeed = modifier->GetRotationSpeed();
    if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, -360.0f, 360.0f, "%.1f deg/s"))
    {
        modifier->SetRotationSpeed(rotationSpeed);
    }
    
    bool randomizeInitialRotation = modifier->GetRandomizeInitialRotation();
    if (ImGui::Checkbox("Random Initial Rotation", &randomizeInitialRotation))
    {
        modifier->SetRandomizeInitialRotation(randomizeInitialRotation);
    }
    
    bool randomizeDirection = modifier->GetRandomizeRotationDirection();
    if (ImGui::Checkbox("Random Direction", &randomizeDirection))
    {
        modifier->SetRandomizeRotationDirection(randomizeDirection);
    }
}

// Helper methods to get modifiers
std::shared_ptr<EmissionShapeModifier> ParticleEditor::GetEmissionShapeModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<EmissionShapeModifier>(
        particleComponent.GetModifier("Emission Shape"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<EmissionShapeModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<GravityModifier> ParticleEditor::GetGravityModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<GravityModifier>(
        particleComponent.GetModifier("Gravity"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<GravityModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<DragModifier> ParticleEditor::GetDragModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<DragModifier>(
        particleComponent.GetModifier("Drag"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<DragModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<VortexModifier> ParticleEditor::GetVortexModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<VortexModifier>(
        particleComponent.GetModifier("Vortex"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<VortexModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<AttractorModifier> ParticleEditor::GetAttractorModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<AttractorModifier>(
        particleComponent.GetModifier("Attractor"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<AttractorModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<SizeOverLifetimeModifier> ParticleEditor::GetSizeOverLifetimeModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<SizeOverLifetimeModifier>(
        particleComponent.GetModifier("Size Over Lifetime"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<SizeOverLifetimeModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<ColorOverLifetimeModifier> ParticleEditor::GetColorOverLifetimeModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<ColorOverLifetimeModifier>(
        particleComponent.GetModifier("Color Over Lifetime"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<ColorOverLifetimeModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

std::shared_ptr<RotationModifier> ParticleEditor::GetRotationModifier(ParticleComponent& particleComponent, bool createIfMissing)
{
    auto modifier = std::dynamic_pointer_cast<RotationModifier>(
        particleComponent.GetModifier("Rotation"));
        
    if (!modifier && createIfMissing)
    {
        modifier = std::make_shared<RotationModifier>();
        particleComponent.AddModifier(modifier);
    }
    
    return modifier;
}

// Update YAML config saving/loading to include modifiers
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
        
        // Enhanced properties
        out << YAML::Key << "rotation" << YAML::Value << state.rotation;
        out << YAML::Key << "angular_velocity" << YAML::Value << state.angularVelocity;
        
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
        out << YAML::Key << "burst_spread" << YAML::Value << state.burstSpread;
        
        // Texture properties
        out << YAML::Key << "use_texture" << YAML::Value << state.useTexture;
        out << YAML::Key << "texture_path" << YAML::Value << state.texturePath;
        
        // Save modifiers state
        out << YAML::Key << "modifiers" << YAML::Value << YAML::BeginMap;
        
        // Emission Shape
        if (auto modifier = GetEmissionShapeModifier(particleComponent))
        {
            out << YAML::Key << "emission_shape" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showEmissionShapeEditor;
            out << YAML::Key << "shape_type" << YAML::Value << static_cast<int>(modifier->GetShapeType());
            out << YAML::Key << "random_direction" << YAML::Value << modifier->GetRandomDirection();
            out << YAML::Key << "direction_angle" << YAML::Value << modifier->GetDirectionAngle();
            out << YAML::Key << "spread_angle" << YAML::Value << modifier->GetSpreadAngle();
            out << YAML::Key << "emit_from_edge" << YAML::Value << modifier->GetEmitFromEdge();
            out << YAML::EndMap;
        }
        
        // Gravity
        if (auto modifier = GetGravityModifier(particleComponent))
        {
            out << YAML::Key << "gravity" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showGravityEditor;
            out << YAML::Key << "gravity_x" << YAML::Value << modifier->GetGravity().x;
            out << YAML::Key << "gravity_y" << YAML::Value << modifier->GetGravity().y;
            out << YAML::Key << "strength" << YAML::Value << modifier->GetStrength();
            out << YAML::EndMap;
        }
        
        // Drag
        if (auto modifier = GetDragModifier(particleComponent))
        {
            out << YAML::Key << "drag" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showDragEditor;
            out << YAML::Key << "coefficient" << YAML::Value << modifier->GetDragCoefficient();
            out << YAML::EndMap;
        }
        
        // Vortex
        if (auto modifier = GetVortexModifier(particleComponent))
        {
            out << YAML::Key << "vortex" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showVortexEditor;
            out << YAML::Key << "center_x" << YAML::Value << modifier->GetCenterOffset().x;
            out << YAML::Key << "center_y" << YAML::Value << modifier->GetCenterOffset().y;
            out << YAML::Key << "rotation_speed" << YAML::Value << modifier->GetRotationSpeed();
            out << YAML::Key << "strength" << YAML::Value << modifier->GetStrength();
            out << YAML::Key << "attraction" << YAML::Value << modifier->GetAttractionStrength();
            out << YAML::EndMap;
        }
        
        // Attractor
        if (auto modifier = GetAttractorModifier(particleComponent))
        {
            out << YAML::Key << "attractor" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showAttractorEditor;
            out << YAML::Key << "position_x" << YAML::Value << modifier->GetAttractorOffset().x;
            out << YAML::Key << "position_y" << YAML::Value << modifier->GetAttractorOffset().y;
            out << YAML::Key << "strength" << YAML::Value << modifier->GetStrength();
            out << YAML::Key << "falloff_radius" << YAML::Value << modifier->GetFalloffRadius();
            out << YAML::Key << "is_repeller" << YAML::Value << modifier->GetIsRepeller();
            out << YAML::EndMap;
        }
        
        // Size Over Lifetime
        if (auto modifier = GetSizeOverLifetimeModifier(particleComponent))
        {
            out << YAML::Key << "size_over_lifetime" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showSizeOverLifetimeEditor;
            out << YAML::Key << "start_scale" << YAML::Value << modifier->GetStartScale();
            out << YAML::Key << "end_scale" << YAML::Value << modifier->GetEndScale();
            out << YAML::Key << "curve_type" << YAML::Value << static_cast<int>(modifier->GetCurveType());
            out << YAML::EndMap;
        }
        
        // Color Over Lifetime
        if (auto modifier = GetColorOverLifetimeModifier(particleComponent))
        {
            out << YAML::Key << "color_over_lifetime" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showColorOverLifetimeEditor;
            
            out << YAML::Key << "start_color" << YAML::Value << YAML::Flow << YAML::BeginSeq 
                << modifier->GetStartColor().r << modifier->GetStartColor().g 
                << modifier->GetStartColor().b << modifier->GetStartColor().a << YAML::EndSeq;
                
            out << YAML::Key << "end_color" << YAML::Value << YAML::Flow << YAML::BeginSeq 
                << modifier->GetEndColor().r << modifier->GetEndColor().g 
                << modifier->GetEndColor().b << modifier->GetEndColor().a << YAML::EndSeq;
                
            out << YAML::EndMap;
        }
        
        // Rotation
        if (auto modifier = GetRotationModifier(particleComponent))
        {
            out << YAML::Key << "rotation" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "enabled" << YAML::Value << state.showRotationEditor;
            out << YAML::Key << "rotation_speed" << YAML::Value << modifier->GetRotationSpeed();
            out << YAML::Key << "random_initial" << YAML::Value << modifier->GetRandomizeInitialRotation();
            out << YAML::Key << "random_direction" << YAML::Value << modifier->GetRandomizeRotationDirection();
            out << YAML::EndMap;
        }
        
        out << YAML::EndMap; // End modifiers
        
        out << YAML::EndMap; // End particle_config
        out << YAML::EndMap; // End root
        
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
                
            // Enhanced properties
            if (particleConfig["rotation"])
                state.rotation = particleConfig["rotation"].as<float>();
                
            if (particleConfig["angular_velocity"])
                state.angularVelocity = particleConfig["angular_velocity"].as<float>();

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
                
            if (particleConfig["burst_spread"])
                state.burstSpread = particleConfig["burst_spread"].as<float>();

            // Texture properties
            if (particleConfig["use_texture"])
                state.useTexture = particleConfig["use_texture"].as<bool>();

            if (particleConfig["texture_path"])
                strncpy(state.texturePath, particleConfig["texture_path"].as<std::string>().c_str(), sizeof(state.texturePath));
                
            // Load texture if needed
            if (state.useTexture)
            {
                // Clear previous texture
                if (state.previewTexture)
                {
                    state.previewTexture = nullptr;
                }
                
                // Try to load the texture
                state.previewTexture = wolf::TextureManager::CreateTexture(state.texturePath);
                if (state.previewTexture)
                {
                    state.textureLoaded = true;
                }
                else
                {
                    state.textureLoaded = false;
                }
            }
            
            // Load modifiers
            if (particleConfig["modifiers"])
            {
                YAML::Node modifiers = particleConfig["modifiers"];
                
                // Clear existing modifiers
                particleComponent.RemoveModifier("Emission Shape");
                particleComponent.RemoveModifier("Gravity");
                particleComponent.RemoveModifier("Drag");
                particleComponent.RemoveModifier("Vortex");
                particleComponent.RemoveModifier("Attractor");
                particleComponent.RemoveModifier("Size Over Lifetime");
                particleComponent.RemoveModifier("Color Over Lifetime");
                particleComponent.RemoveModifier("Rotation");
                
                // Emission Shape
                if (modifiers["emission_shape"])
                {
                    YAML::Node emissionShape = modifiers["emission_shape"];
                    state.showEmissionShapeEditor = emissionShape["enabled"].as<bool>();
                    
                    if (state.showEmissionShapeEditor)
                    {
                        auto modifier = GetEmissionShapeModifier(particleComponent, true);
                        modifier->SetShapeType(static_cast<EmissionShapeModifier::ShapeType>(
                            emissionShape["shape_type"].as<int>()));
                        modifier->SetRandomDirection(emissionShape["random_direction"].as<bool>());
                        modifier->SetDirectionAngle(emissionShape["direction_angle"].as<float>());
                        modifier->SetSpreadAngle(emissionShape["spread_angle"].as<float>());
                        modifier->SetEmitFromEdge(emissionShape["emit_from_edge"].as<bool>());
                    }
                }
                
                // Gravity
                if (modifiers["gravity"])
                {
                    YAML::Node gravity = modifiers["gravity"];
                    state.showGravityEditor = gravity["enabled"].as<bool>();
                    
                    if (state.showGravityEditor)
                    {
                        auto modifier = GetGravityModifier(particleComponent, true);
                        modifier->SetGravity(glm::vec2(
                            gravity["gravity_x"].as<float>(),
                            gravity["gravity_y"].as<float>()
                        ));
                        modifier->SetStrength(gravity["strength"].as<float>());
                    }
                }
                
                // Drag
                if (modifiers["drag"])
                {
                    YAML::Node drag = modifiers["drag"];
                    state.showDragEditor = drag["enabled"].as<bool>();
                    
                    if (state.showDragEditor)
                    {
                        auto modifier = GetDragModifier(particleComponent, true);
                        modifier->SetDragCoefficient(drag["coefficient"].as<float>());
                    }
                }
                
                // Vortex
                if (modifiers["vortex"])
                {
                    YAML::Node vortex = modifiers["vortex"];
                    state.showVortexEditor = vortex["enabled"].as<bool>();
                    
                    if (state.showVortexEditor)
                    {
                        auto modifier = GetVortexModifier(particleComponent, true);
                        modifier->SetCenterOffset(glm::vec2(
                            vortex["center_x"].as<float>(),
                            vortex["center_y"].as<float>()
                        ));
                        modifier->SetRotationSpeed(vortex["rotation_speed"].as<float>());
                        modifier->SetStrength(vortex["strength"].as<float>());
                        modifier->SetAttractionStrength(vortex["attraction"].as<float>());
                    }
                }
                
                // Attractor
                if (modifiers["attractor"])
                {
                    YAML::Node attractor = modifiers["attractor"];
                    state.showAttractorEditor = attractor["enabled"].as<bool>();
                    
                    if (state.showAttractorEditor)
                    {
                        auto modifier = GetAttractorModifier(particleComponent, true);
                        modifier->SetAttractorOffset(glm::vec2(
                            attractor["position_x"].as<float>(),
                            attractor["position_y"].as<float>()
                        ));
                        modifier->SetStrength(attractor["strength"].as<float>());
                        modifier->SetFalloffRadius(attractor["falloff_radius"].as<float>());
                        modifier->SetIsRepeller(attractor["is_repeller"].as<bool>());
                    }
                }
                
                // Size Over Lifetime
                if (modifiers["size_over_lifetime"])
                {
                    YAML::Node sizeOverLifetime = modifiers["size_over_lifetime"];
                    state.showSizeOverLifetimeEditor = sizeOverLifetime["enabled"].as<bool>();
                    
                    if (state.showSizeOverLifetimeEditor)
                    {
                        auto modifier = GetSizeOverLifetimeModifier(particleComponent, true);
                        modifier->SetScales(
                            sizeOverLifetime["start_scale"].as<float>(),
                            sizeOverLifetime["end_scale"].as<float>()
                        );
                        modifier->SetCurveType(static_cast<SizeOverLifetimeModifier::CurveType>(
                            sizeOverLifetime["curve_type"].as<int>()));
                    }
                }
                
                // Color Over Lifetime
                if (modifiers["color_over_lifetime"])
                {
                    YAML::Node colorOverLifetime = modifiers["color_over_lifetime"];
                    state.showColorOverLifetimeEditor = colorOverLifetime["enabled"].as<bool>();
                    
                    if (state.showColorOverLifetimeEditor)
                    {
                        auto startColor = colorOverLifetime["start_color"].as<std::vector<float>>();
                        auto endColor = colorOverLifetime["end_color"].as<std::vector<float>>();
                        
                        auto modifier = GetColorOverLifetimeModifier(particleComponent, true);
                        modifier->SetColors(
                            glm::vec4(startColor[0], startColor[1], startColor[2], startColor[3]),
                            glm::vec4(endColor[0], endColor[1], endColor[2], endColor[3])
                        );
                    }
                }
                
                // Rotation
                if (modifiers["rotation"])
                {
                    YAML::Node rotation = modifiers["rotation"];
                    state.showRotationEditor = rotation["enabled"].as<bool>();
                    
                    if (state.showRotationEditor)
                    {
                        auto modifier = GetRotationModifier(particleComponent, true);
                        modifier->SetRotationSpeed(rotation["rotation_speed"].as<float>());
                        modifier->SetRandomizeInitialRotation(rotation["random_initial"].as<bool>());
                        modifier->SetRandomizeRotationDirection(rotation["random_direction"].as<bool>());
                    }
                }
            }

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

void ParticleEditor::ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state)
{
    particleComponent.SetMaxParticles(state.maxParticles);
}