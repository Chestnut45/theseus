#pragma once

#include <W_BaseComponent.h>
#include "ParticleComponent.h"
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <unordered_map>

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
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float size = 5.0f;
        float lifetime = 2.0f;
        glm::vec2 velocity{0.0f, 10.0f};
        size_t maxParticles = 100;
        std::string configFilePath = "data/particles/default.yaml";
    };

    bool editorVisible = false;

    std::unordered_map<ParticleComponent*, EditorState> m_editorStates;

    void ApplyEditorSettings(ParticleComponent& particleComponent, EditorState& state);
};
