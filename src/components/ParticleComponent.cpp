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
        else
        {
            wolf::Log("Particle shader program created successfully");
        }
    }
    s_refCount++;

    InitGLResources();
    InitQuadResources();
    
    // Debug quad setup, trying to figure out the current binding and tex cords
    GLint vao, attrib;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib);
    // wolf::Log("Quad VAO check: current binding=", vao, 
    //          ", texture coord attribute enabled=", attrib ? "YES" : "NO");
}

ParticleComponent::~ParticleComponent()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }

    // Don't destroy textures here either - TextureManager should handle their lifetime, just reminding myself
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

    // Check if texture coord attribute is enabled while VAO is still bound
    GLint attrib;
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib);
    // wolf::Log("Texture coord attribute enabled check (during binding): ", attrib ? "YES" : "NO");

    // Now unbind the VAO
    glBindVertexArray(0);
    
    // Test binding and verification
    glBindVertexArray(m_quadVAO);
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib);
    // wolf::Log("Texture coord attribute enabled check (after rebind): ", attrib ? "YES" : "NO");
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
    
    int texturedParticleCount = 0; // Debug counter
    
    // Count and group active textured particles
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && particle.m_texture)
        {
            textureGroups[particle.m_texture].push_back(&particle);
            texturedParticleCount++;
        }
    }
    
    if (textureGroups.empty())
    {
        // Debug output
        static bool reported = false;
        if (!reported && texturedParticleCount == 0)
        {
            wolf::Log("No textured particles to render");
            reported = true;
        }
        return;
    }
    
    // Debug output
    static bool first_render = true;
    if (first_render)
    {
        // wolf::Log("Rendering textured particles. Count: ", texturedParticleCount);
        // wolf::Log("Number of unique textures: ", textureGroups.size());
    }
    
    glBindVertexArray(m_quadVAO);
    
    // Just try to set the uniforms - we can't check if they exist but we can see if rendering works
    try {
        s_pShader->SetUniform("useTexture", 1);
        s_pShader->SetUniform("particleTexture", 0);
        
        // Render particles grouped by texture to minimize state changes
        for (const auto& [texture, particles] : textureGroups)
        {
            // Debug: Ensure texture is valid
            if (!texture || texture->GetID() == 0)
            {
                wolf::Error("Invalid texture detected!");
                continue;
            }
            
            // Bind texture once per group
            glActiveTexture(GL_TEXTURE0);
            texture->Bind(0);
            
            // Debug: Confirm texture binding
            GLint currentTexture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
            if (first_render)
            {
                // wolf::Log("Texture binding: texture ID=", texture->GetID(), ", GL texture=", currentTexture);
                // wolf::Log("Number of particles in this texture group: ", particles.size());
            }
            
            for (const Particle* particle : particles)
            {
                // Set up model matrix for this particle with fixed multiplier for size
                // Scale factor adjusted to make particles more visible
                float scaleFactor = particle->m_size * 10.0f; // Increase size by a factor of 10
                
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(particle->m_pos, 0.0f));
                model = glm::scale(model, glm::vec3(scaleFactor, scaleFactor, 1.0f));
                
                // Set particle-specific uniforms
                s_pShader->SetUniform("model", model);
                s_pShader->SetUniform("particleColor", particle->m_color);
                
                // Draw the quad
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Debug: Check for OpenGL errors after first draw
                if (first_render)
                {
                    GLenum err = glGetError();
                    if (err != GL_NO_ERROR)
                    {
                        wolf::Error("OpenGL error during particle rendering: ", err);
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        wolf::Error("Exception during textured particle rendering: ", e.what());
    }
    
    first_render = false;
}

void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime, wolf::Texture* texture)
{
    // Let's find how many active particles we have to debug the emission
    int activeCount = 0;
    for (const auto& p : m_particles)
    {
        if (p.m_active) activeCount++;
    }
    
    // Log the current state
    // wolf::Log("Emitting particle. Current active particles: ", activeCount, "/", m_particles.size());
    
    // Try to find an inactive particle
    bool emitted = false;
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            particle.Reset(position, velocity, color, size, lifetime, texture);
            emitted = true;
            
            if (texture)
            {
                // wolf::Log("Emitted textured particle with texture ID: ", texture->GetID());
            }
            break; // Important: Only use the first available particle, then exit
        }
    }
    
    // If we couldn't emit, log an error
    if (!emitted)
    {
        wolf::Error("Failed to emit particle - all particles are active. Consider increasing max particles.");
    }
}

void ParticleComponent::SetMaxParticles(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}