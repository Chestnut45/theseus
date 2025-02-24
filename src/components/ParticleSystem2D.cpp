#include "ParticleSystem2D.h"
#include <iostream>

// Constructor
ParticleSystem2D::ParticleSystem2D()
{
    if (s_refCount == 0)
    {
        s_pShader = wolf::ProgramManager::CreateProgram("shaders/particle2D.vs", "shaders/particle2D.fs");
    }
    s_refCount++;

    InitGLResources();
}

// Destructor
ParticleSystem2D::~ParticleSystem2D()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }

    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_instanceVBO);
    glDeleteVertexArrays(1, &m_vao);
}

// Register a ParticleComponent to be managed
void ParticleSystem2D::RegisterComponent(ParticleComponent* component)
{
    m_components.insert(component);
}

// Unregister a ParticleComponent
void ParticleSystem2D::UnregisterComponent(ParticleComponent* component)
{
    m_components.erase(component);
}

// Update all registered particle components
void ParticleSystem2D::Update(float delta)
{
    for (auto* component : m_components)
    {
        component->Update(delta);
    }
}

// Renders all active particles from registered components
void ParticleSystem2D::Render()
{
    if (!s_pShader) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    s_pShader->Bind();
    glBindVertexArray(m_vao);

    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    std::vector<float> sizes;

    for (auto* component : m_components)
    {
        for (const auto& particle : component->GetParticles())
        {
            if (particle.m_active)
            {
                positions.push_back(particle.m_pos);
                colors.push_back(particle.m_color);
                sizes.push_back(particle.m_size);
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

    glDrawArrays(GL_POINTS, 0, positions.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

// Initializes OpenGL resources for rendering
void ParticleSystem2D::InitGLResources()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Vertex buffer (positions)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    // Instance buffer (colors + size)
    glGenBuffers(1, &m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
}

// Shows ImGui editor for controlling particles
void ParticleSystem2D::ShowEditor()
{
    ImGui::Begin("Particle System Editor");

    ImGui::SeparatorText("Particle Properties");
    ImGui::ColorEdit4("Particle Color", &m_editorColor.r);
    ImGui::SliderFloat("Size", &m_editorSize, 1.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Lifetime", &m_editorLifetime, 0.1f, 5.0f, "%.1f");

    ImGui::SeparatorText("Spawn Controls");
    ImGui::SliderFloat2("Velocity", &m_editorVelocity.x, -20.0f, 20.0f, "%.1f");

    if (ImGui::Button("Spawn Particle"))
    {
        for (auto* component : m_components)
        {
            component->Emit({0.0f, 0.0f}, m_editorVelocity, m_editorColor, m_editorSize, m_editorLifetime);
        }
    }

    ImGui::SeparatorText("System Controls");
    if (ImGui::Button("Clear Particles"))
    {
        for (auto* component : m_components)
        {
            for (auto& particle : component->GetParticles())
            {
                particle.m_active = false;
            }
        }
    }

    ImGui::End();
}
