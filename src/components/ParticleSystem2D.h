//-----------------------------------------------------------------------------
// File:			ParticleSystem2D.h
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for managing all instances of particles
//-----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <unordered_set>
#include <GL/glew.h>
#include <W_ProgramManager.h>
#include <imgui/imgui.h>
#include "ParticleComponent.h"
#include "W_GameObject.h"
#include "W_Transform2D.h"

class ParticleSystem2D
{
public:
    ParticleSystem2D();
    ~ParticleSystem2D();

    void RegisterComponent(ParticleComponent* component);
    void UnregisterComponent(ParticleComponent* component);
    
    void Update(float delta);
    void Render();

    // ImGui Debug Editor
    void ShowEditor();
    void ToggleEditor() { m_showEditor = !m_showEditor; }  // New toggle function
    bool IsEditorOpen() const { return m_showEditor; }

private:
    std::unordered_set<ParticleComponent*> m_components;

    // ImGui controlled parameters
    glm::vec4 m_editorColor{1.0f, 1.0f, 1.0f, 1.0f}; // White
    float m_editorSize = 5.0f;
    float m_editorLifetime = 2.0f;
    glm::vec2 m_editorVelocity{0.0f, 10.0f};

    GLuint m_vao, m_vbo, m_instanceVBO;
    static inline wolf::Program* s_pShader = nullptr;
    static inline size_t s_refCount = 0;

    GLuint m_quadVBO;
    GLuint m_posVBO;
    GLuint m_colorVBO;
    GLuint m_sizeVBO;

    void InitGLResources();
    void TrackNewComponents(); // New function for tracking dynamically added components

    bool m_showEditor = false;  // Store the UI state inside the system

};
