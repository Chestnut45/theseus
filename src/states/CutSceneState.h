#pragma once
#include "GameState.h"
#include <glm/vec2.hpp>
#include <glm/glm.hpp>
#include "theseus.h"
#include "GameStateManager.h"
#include <vector>
#include <yaml-cpp/yaml.h>
#include <string>

class CutSceneState : public GameState {
public:
    // Add cutsceneID parameter to constructor
    CutSceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& scriptPath, const std::string& cutsceneID)
        : GameState(manager, gameInstance), m_scriptPath(scriptPath), m_cutsceneID(cutsceneID) {}

    void Enter() override;
    void Exit() override;
    void Pause() override {}
    void Resume() override {}
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override {}
    void BackgroundRender() override {}

private:
    struct CameraKeyframe {
        glm::vec2 position;
        float zoom;
        float duration;
    };

    std::string m_scriptPath;
    std::string m_cutsceneID;  // ID for the specific cutscene to load
    std::vector<CameraKeyframe> m_cameraKeyframes;
    int m_currentKeyframe = 0;
    float m_timeSinceLastKeyframe = 0.0f;
    glm::vec2 m_currentCameraPosition;
    float m_currentZoomLevel;
    glm::vec2 m_initialPosition;
    float m_initialZoom;
};
