#include "ParticleComponent.h"
#include <W_ProgramManager.h>
#include <iostream>

ParticleComponent::ParticleComponent(size_t maxParticles)
{
    m_particles.resize(maxParticles);

    if (s_refCount == 0)
    {
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/particle2D.vs", "data/shaders/particle2D.fs");
    }
    s_refCount++;

    InitGLResources();
}

ParticleComponent::~ParticleComponent()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }
    CleanupGLResources();
}

void ParticleComponent::InitGLResources()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &m_sizeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    glEnable(GL_PROGRAM_POINT_SIZE);
}

void ParticleComponent::CleanupGLResources()
{
    glDeleteBuffers(1, &m_posVBO);
    glDeleteBuffers(1, &m_colorVBO);
    glDeleteBuffers(1, &m_sizeVBO);
    glDeleteVertexArrays(1, &m_vao);
}

void ParticleComponent::Update(float delta)
{
    for (auto& particle : m_particles)
    {
        if (particle.m_active)
        {
            particle.Update(delta);
        }
    }
}

void ParticleComponent::Render()
{
    if (!s_pShader) return;

    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    std::vector<float> sizes;

    for (const auto& particle : m_particles)
    {
        if (particle.m_active)
        {
            positions.push_back(particle.m_pos);
            colors.push_back(particle.m_color);
            sizes.push_back(particle.m_size);
        }
    }

    if (positions.empty()) return;

    s_pShader->Bind();
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_STREAM_DRAW);

    glPointSize(10.0f); 

    glDrawArrays(GL_POINTS, 0, positions.size());

    glBindVertexArray(0);
}

void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime)
{
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            particle.Reset(position, velocity, color, size, lifetime);
            return;
        }
    }
}

void ParticleComponent::SetMaxParticles(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}
