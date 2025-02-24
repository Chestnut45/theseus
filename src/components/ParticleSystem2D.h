#pragma once

#include <vector>
#include <unordered_set>
#include <GL/glew.h>
#include <W_ProgramManager.h>
#include <imgui/imgui.h>
#include "ParticleComponent.h"

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

    void InitGLResources();
};
