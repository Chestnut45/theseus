#include "ParticleComponent.h"
#include <W_ProgramManager.h>
#include <unordered_map>
#include <algorithm>
#include <W_Logging.h>

ParticleComponent::ParticleComponent(size_t maxParticles)
{
    m_particles.resize(maxParticles);

    if (s_refCount == 0)
    {
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/particle2D.vs", "data/shaders/particle2D.fs");
        
        if (!s_pShader)
        {
            wolf::Error("Failed to create particle shader program!");
        }
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

    // Don't destroy textures here - TextureManager should handle their lifetime
    // Just clear the pointers
    for (auto& particle : m_particles)
    {
        particle.m_texture = nullptr;
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
    // Define a unit quad with texture coordinates
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

    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute (location 3)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(3);

    // Now unbind the VAO
    glBindVertexArray(0);
}

void ParticleComponent::CleanupGLResources()
{
    glDeleteBuffers(1, &m_posVBO);
    glDeleteBuffers(1, &m_colorVBO);
    glDeleteBuffers(1, &m_sizeVBO);
    glDeleteVertexArrays(1, &m_vao);
    
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteBuffers(1, &m_quadEBO);
    glDeleteVertexArrays(1, &m_quadVAO);
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

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    s_pShader->Bind();

    // Render non-textured particles as points
    RenderPointParticles();
    
    // Render textured particles using quads, grouped by texture for better performance
    RenderTexturedParticles();

    // Cleanup state
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void ParticleComponent::RenderPointParticles()
{
    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    std::vector<float> sizes;
    
    // Collect active non-textured particles
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && !particle.m_texture)
        {
            positions.push_back(particle.m_pos);
            colors.push_back(particle.m_color);
            sizes.push_back(particle.m_size);
        }
    }
    
    if (positions.empty())
        return;
        
    glBindVertexArray(m_vao);
    
    // Update VBOs with new data
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_STREAM_DRAW);
    
    // Set shader uniforms for point particles
    s_pShader->SetUniform("useTexture", 0);
    
    // Draw all point particles in one batch
    glDrawArrays(GL_POINTS, 0, positions.size());
}

void ParticleComponent::RenderTexturedParticles()
{
    // Group particles by texture for efficient rendering
    std::unordered_map<wolf::Texture*, std::vector<const Particle*>> textureGroups;
    
    // Group active textured particles
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && particle.m_texture)
        {
            textureGroups[particle.m_texture].push_back(&particle);
        }
    }
    
    if (textureGroups.empty())
        return;
    
    // Make sure shader is bound
    s_pShader->Bind();
    
    glBindVertexArray(m_quadVAO);
    
    // Set useTexture uniform using the Wolf API
    s_pShader->SetUniform("useTexture", 1);
    s_pShader->SetUniform("particleTexture", 0); 
    
    // Render particles grouped by texture to minimize state changes
    for (const auto& [texture, particles] : textureGroups)
    {
        // Skip invalid textures
        if (!texture || texture->GetID() == 0)
            continue;
        
        // Bind texture once per group
        glActiveTexture(GL_TEXTURE0);
        texture->Bind(0);
        
        for (const Particle* particle : particles)
        {
            // Set up model matrix for this particle 
            float scaleFactor = particle->m_size * 10.0f; // Adjust size scaling as needed
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(particle->m_pos, 0.0f));
            model = glm::scale(model, glm::vec3(scaleFactor, scaleFactor, 1.0f));
            
            // Set particle-specific uniforms using the Wolf API
            s_pShader->SetUniform("model", model);
            s_pShader->SetUniform("particleColor", particle->m_color);
            
            // Important: Re-bind to upload the new uniform values
            s_pShader->Bind();
            
            // Draw the quad
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
}

void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime, wolf::Texture* texture)
{
    // Try to find an inactive particle
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            particle.Reset(position, velocity, color, size, lifetime, texture);
            return; // Found and used an inactive particle, exit
        }
    }
    
    // If we reached here, all particles are active
    wolf::Error("Failed to emit particle - all particles are active. Consider increasing max particles.");
}

void ParticleComponent::SetMaxParticles(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}