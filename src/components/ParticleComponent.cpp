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
    InitQuadResources();
}

ParticleComponent::~ParticleComponent()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }

    for (auto& particle : m_particles)
    {
        if (particle.m_texture)
        {
            wolf::TextureManager::DestroyTexture(particle.m_texture);
            particle.m_texture = nullptr;
        }
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

void ParticleComponent::InitQuadResources()
{
    GLfloat vertices[] = {
        // Positions  // TexCoords
        -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
         0.5f, -0.5f,  1.0f, 0.0f, // Bottom-right
         0.5f,  0.5f,  1.0f, 1.0f, // Top-right
        -0.5f,  0.5f,  0.0f, 1.0f  // Top-left
    };

    GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
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

    s_pShader->Bind();

    // Enable blending for transparency if needed
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render GL_POINTS for non-textured particles
    glBindVertexArray(m_vao);
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && !particle.m_texture)
        {
            positions.push_back(particle.m_pos);
            colors.push_back(particle.m_color);
            sizes.push_back(particle.m_size);
        }
    }

    if (!positions.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_STREAM_DRAW);

        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, positions.size());
    }

    // Render quads for textured particles
    glBindVertexArray(m_quadVAO);
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && particle.m_texture)
        {
            glActiveTexture(GL_TEXTURE0);
            particle.m_texture->Bind(0);
            std::cout << "Binding Texture ID: " << particle.m_texture->GetID() << std::endl;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(particle.m_pos, 0.0f));
            model = glm::scale(model, glm::vec3(particle.m_size, particle.m_size, 1.0f));

            s_pShader->SetUniform("model", model);
            s_pShader->SetUniform("particleColor", particle.m_color);
            s_pShader->SetUniform("useTexture", 1);
            s_pShader->SetUniform("particleTexture", 0);

            std::cout << "Rendering textured particle at: (" << particle.m_pos.x << ", " << particle.m_pos.y << ")" << std::endl;

            GLint currentTexture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
            std::cout << "Active Texture ID: " << currentTexture << std::endl;

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime, wolf::Texture* texture)
{
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            std::cout << "Emitting particle with texture: " 
                      << (texture ? texture->GetID() : -1) << std::endl;

            particle.Reset(position, velocity, color, size, lifetime, texture);
            return;
        }
    }
}

void ParticleComponent::SetMaxParticles(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}
