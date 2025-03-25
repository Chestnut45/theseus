#pragma once

#include <W_BaseComponent.h>
#include "ParticleComponent.h"
#include "ParticleModifier.h"
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
        float rotation = 0.0f;
        float angularVelocity = 0.0f;
        
        // System properties
        size_t maxParticles = 100;
        float emissionRate = 5.0f;
        float emissionTimer = 0.0f;
        bool continuousEmission = false;
        int burstCount = 10;
        float burstSpread = 360.0f;
        
        // Configuration
        std::string configFilePath = "data/particles/default.yaml";

        // Texture properties
        bool useTexture = false;
        char texturePath[256] = "data/particles/textures/default.png";
        
        // Cache for texture preview and emission
        wolf::Texture* previewTexture = nullptr;
        bool textureLoaded = false;
        
        // Modifiers enabled states
        bool showEmissionShapeEditor = false;
        bool showGravityEditor = false;
        bool showDragEditor = false;
        bool showVortexEditor = false;
        bool showAttractorEditor = false;
        bool showSizeOverLifetimeEditor = false;
        bool showColorOverLifetimeEditor = false;
        bool showRotationEditor = false;
    };

    bool m_editorVisible = false;
    std::unordered_map<ParticleComponent*, EditorState> m_editorStates;

    // Helper methods
    void ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state);
    void EmitParticle(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position);
    void EmitParticleBurst(ParticleComponent& particleComponent, EditorState& state, const glm::vec2& position, int count);
    int CountActiveParticles(ParticleComponent& particleComponent);
    
    // New modifier editor UI methods
    void ShowModifiersPanel(ParticleComponent& particleComponent, EditorState& state);
    void ShowEmissionShapeEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowGravityEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowDragEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowVortexEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowAttractorEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowSizeOverLifetimeEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowColorOverLifetimeEditor(ParticleComponent& particleComponent, EditorState& state);
    void ShowRotationEditor(ParticleComponent& particleComponent, EditorState& state);
    
    // Helper methods to get modifiers
    std::shared_ptr<EmissionShapeModifier> GetEmissionShapeModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<GravityModifier> GetGravityModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<DragModifier> GetDragModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<VortexModifier> GetVortexModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<AttractorModifier> GetAttractorModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<SizeOverLifetimeModifier> GetSizeOverLifetimeModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<ColorOverLifetimeModifier> GetColorOverLifetimeModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
    std::shared_ptr<RotationModifier> GetRotationModifier(ParticleComponent& particleComponent, bool createIfMissing = false);
};