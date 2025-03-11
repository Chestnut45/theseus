#pragma once

#include <W_BaseComponent.h>
#include "ParticleComponent.h"
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <W_TextureManager.h>

class ParticleEditor : public wolf::BaseComponent
{
public:
    ParticleEditor() = default;
    ~ParticleEditor() = default;

    void Update(float delta);
    void ShowEditor();
    void SaveConfigToYAML(ParticleComponent& particleComponent, const std::string& filename);
    void LoadConfigFromYAML(ParticleComponent& particleComponent, const std::string& filename);

private:
    struct EditorState
    {
        // Basic particle properties
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float size = 5.0f;
        float lifetime = 2.0f;
        glm::vec2 velocity{0.0f, 10.0f};
        
        // System properties
        size_t maxParticles = 100;
        float emissionRate = 5.0f;
        float emissionTimer = 0.0f;
        bool continuousEmission = false;
        int burstCount = 10;
        
        // Configuration
        std::string configFilePath = "data/particles/default.yaml";

        // Texture properties
        bool useTexture = false;
        char texturePath[256] = "data/particles/textures/default.png";
        
        // Cache for texture preview and emission
        wolf::Texture* previewTexture = nullptr;
        bool textureLoaded = false;
    };

    bool m_editorVisible = false;
    std::unordered_map<ParticleComponent*, EditorState> m_editorStates;

    // Helper methods
    void ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state);
    void EmitParticle(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position);
    void EmitParticleBurst(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position, int count);
    int CountActiveParticles(ParticleComponent& particleComponent);
};