#include "BoundedFluidSystem2D.h"

#include <glm/gtx/norm.hpp>
#include <imgui/imgui.h>
#include <W_Logging.h>
#include <W_Transform2D.h>

BoundedFluidSystem2D::BoundedFluidSystem2D(const wolf::Rectangle& bounds, int numParticles)
    :
    m_bounds(bounds),
    m_numParticlesToSpawn(numParticles)
{
    // Reference counting
    if (s_refCount == 0)
    {
        // Initialize static resources

        // Dummy VAO for post processing pass
        glGenVertexArrays(1, &s_dummyVAO);

        // Upload quad vertex data
        float quadVerts[] = {
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
        };
        glGenBuffers(1, &s_quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        
        // Generate quad vao
        glGenVertexArrays(1, &s_quadVAO);
        glBindVertexArray(s_quadVAO);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind quad VBO

        // Create particle data SSBO
        // TODO: Account for initial size?
        glGenBuffers(1, &s_particleSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_particleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(FluidParticle) * m_numParticlesToSpawn, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Create shaders
        s_pParticleShader = wolf::ProgramManager::CreateProgram("data/shaders/fluid_particle.vs", "data/shaders/fluid_particle.fs");
        s_pBlendPassShader = wolf::ProgramManager::CreateProgram("data/shaders/fullscreen_pass.vs", "data/shaders/fluid_blend_pass.fs");

        // Create framebuffer
        glGenFramebuffers(1, &s_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
        glGenTextures(1, &s_fbColorTex);
        glBindTexture(GL_TEXTURE_2D, s_fbColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_fbColorTex, 0);

        // Ensure completeness
        if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            wolf::Error("Fluid sim framebuffer not complete!");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    s_refCount++;
}

BoundedFluidSystem2D::~BoundedFluidSystem2D()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        // Cleanup static resources
        glDeleteFramebuffers(1, &s_framebuffer);
        glDeleteTextures(1, &s_fbColorTex);
        glDeleteBuffers(1, &s_quadVBO);
        glDeleteBuffers(1, &s_particleSSBO);
        glDeleteVertexArrays(1, &s_quadVAO);
        wolf::ProgramManager::DestroyProgram(s_pParticleShader);
        wolf::ProgramManager::DestroyProgram(s_pBlendPassShader);
        s_pParticleShader = nullptr;
        glDeleteVertexArrays(1, &s_dummyVAO);
    }
}

void BoundedFluidSystem2D::Update(float delta)
{
    // Update total simulation time and elapsed counter
    m_simTime += delta;
    m_elapsedTime += delta;
    
    // Don't step simulation until target frame time is hit
    if (m_elapsedTime < m_targetFrametime) return;

    // Reset counter if we step this frame
    m_elapsedTime = 0.0f;

    // Update spouts
    for (int i = 0; i < m_spouts.size(); ++i)
    {
        auto& spout = m_spouts[i];
        spout.m_lifetime += m_targetFrametime;
        spout.m_spawnTimer += m_targetFrametime;
        
        // Spawn new particles
        while (spout.m_spawnTimer > spout.m_spawnRate)
        {
            spout.m_spawnTimer -= spout.m_spawnRate;
            glm::vec2 randomOffset = glm::vec2(m_rng.NextFloat(-0.01f, 0.01f), m_rng.NextFloat(-0.01f, 0.01f));
            SpawnParticle(spout.m_pos + randomOffset, randomOffset);
        }

        // Delete spouts that are done
        if (spout.m_lifetime >= spout.m_lifespan)
        {
            m_spouts.erase(m_spouts.begin() + i);
            i--;
        }
    }

    static const glm::ivec2 adjacentCellOffsets[] = {
        glm::ivec2(-1, 0),
        glm::ivec2(-1, -1),
        glm::ivec2(-1, 1),
        glm::ivec2(1, 0),
        glm::ivec2(1, -1),
        glm::ivec2(1, 1),
        glm::ivec2(0, -1),
        glm::ivec2(0, 1),
        glm::ivec2(0, 0) // Adding the 0 offset here simplifies the code below
    };

    // Update spatial map
    m_spatialMap.clear();
    int particleIndex = 0;
    for (auto& p : m_particles)
    {
        auto gridCell = GetGridCell(p.m_pos);
        m_spatialMap[gridCell].push_back(particleIndex);
        particleIndex++;
    }

    // Compute the density and pressure of each particle
    for (auto& particle : m_particles)
    {
        particle.m_density = 0.0f;

        // Get the current grid cell
        glm::ivec2 gridCell = GetGridCell(particle.m_pos);

        // Iterate all adjacent cells
        for (int neighbourCell = 0; neighbourCell < 9; ++neighbourCell)
        {
            // Grab the list of particle indices from the spatial grid
            auto& cellParticleIndices = m_spatialMap[gridCell + adjacentCellOffsets[neighbourCell]];

            // Sum density contributions from other particles in nearby grid cells
            for (int i = 0; i < cellParticleIndices.size(); ++i)
            {
                // Grab the other particle
                auto& other = m_particles[cellParticleIndices[i]];

                // Ensure it's close enough to count
                glm::vec2 between = other.m_pos - particle.m_pos;
                float sqrDist = glm::length2(between);
                if (sqrDist < m_kernelRadiusSqr)
                {
                    particle.m_density += m_particleMass * m_poly6 * pow(m_kernelRadiusSqr - sqrDist, 3.0f);
                }
            }
        }

        // Calculate pressure
        particle.m_pressure = m_gasConstant * (particle.m_density - m_restDensity);
    }

    // Compute forces acting on each particle
    for (auto& particle : m_particles)
    {
        glm::vec2 pressureForce(0.0f);
        glm::vec2 viscosityForce(0.0f);

        // Get the current grid cell
        glm::ivec2 gridCell = GetGridCell(particle.m_pos);

        // Iterate all adjacent cells
        for (int neighbourCell = 0; neighbourCell < 9; ++neighbourCell)
        {
            // Grab the list of particle indices from the spatial grid
            auto& cellParticleIndices = m_spatialMap[gridCell + adjacentCellOffsets[neighbourCell]];

            // Sum density contributions from other particles in nearby grid cells
            for (int i = 0; i < cellParticleIndices.size(); ++i)
            {
                // Grab the other particle
                auto& other = m_particles[cellParticleIndices[i]];

                if (&particle == &other) continue;

                // Ensure it's close enough to count
                glm::vec2 between = other.m_pos - particle.m_pos;
                float dist = glm::length(between);

                // TODO: A normalization issue can cause -nan positions which pollutes a single
                // spatial grid cell with "dead" particles, killing performance. Fix!
                // NOTE: Potentially ignore any dist < 1e-6?
                if (dist < m_kernelRadius)
                {
                    pressureForce += -glm::normalize(between) * m_particleMass * (particle.m_pressure + other.m_pressure) / (2.0f * other.m_density) * m_spikyGradient * (float)pow(m_kernelRadius - dist, 3.0f);
                    viscosityForce += m_viscosity * m_particleMass * (other.m_vel - particle.m_vel) / other.m_density * m_viscLaplacian * (m_kernelRadius - dist);
                }
            }
        }

        // Calculate force of gravity and add final forces together
        glm::vec2 gravityForce = m_simulateGravity ? m_gravity * m_particleMass / particle.m_density : glm::vec2(0.0f);
        particle.m_force = pressureForce + viscosityForce + gravityForce;
    }

    // Integrate particles using fixed time step
    // TODO: Try leapfrog integration? (numerical stability)
    for (auto& p : m_particles)
    {
        // Forward Euler integration
        p.m_vel += m_fixedDelta * p.m_force / p.m_density;
        p.m_pos += m_fixedDelta * p.m_vel;

        // Static collision rectangle boundary enforcement
        for (const auto& rect : m_collisionRects)
        {
            // Find the closest point on the rectangle to the circle
            float closestX = std::max(rect.m_left, std::min(p.m_pos.x, rect.m_right));
            float closestY = std::max(rect.m_bottom, std::min(p.m_pos.y, rect.m_top));

            // Compute delta
            float deltaX = p.m_pos.x - closestX;
            float deltaY = p.m_pos.y - closestY;
            float distanceSquared = deltaX * deltaX + deltaY * deltaY;

            // Check if there is a collision
            if (distanceSquared < pow(m_boundEpsilon, 2))
            {
                float distance = std::sqrt(distanceSquared);
                if (distance == 0.0f)
                {
                    // TODO: handle...
                }
                else
                {
                    float normalX = deltaX / distance;
                    float normalY = deltaY / distance;
                    if (std::abs(deltaX) > std::abs(deltaY))
                    {
                        p.m_pos.x += normalX * (m_boundEpsilon - std::abs(deltaX));
                        p.m_vel.x *= m_boundDamping;
                    }
                    else
                    {
                        p.m_pos.y += normalY * (m_boundEpsilon - std::abs(deltaY));
                        p.m_vel.y *= m_boundDamping;
                    }
                }
            }
        }

        // Boundary enforcement
        if (p.m_pos.x - m_boundEpsilon < m_bounds.m_left)
        {
            p.m_vel.x *= m_boundDamping;
            p.m_pos.x = m_bounds.m_left + m_boundEpsilon;
        }
        if (p.m_pos.x + m_boundEpsilon > m_bounds.m_right)
        {
            p.m_vel.x *= m_boundDamping;
            p.m_pos.x = m_bounds.m_right - m_boundEpsilon;
        }
        if (p.m_pos.y - m_boundEpsilon < m_bounds.m_bottom)
        {
            p.m_vel.y *= m_boundDamping;
            p.m_pos.y = m_bounds.m_bottom + m_boundEpsilon;
        }
        if (p.m_pos.y + m_boundEpsilon > m_bounds.m_top)
        {
            p.m_vel.y *= m_boundDamping;
            p.m_pos.y = m_bounds.m_top - m_boundEpsilon;
        }
    }
}

void BoundedFluidSystem2D::Render(float delta)
{   
    // Upload particle data to SSBO
    // TODO: Could switch to a persistently mapped buffer with double buffering for better perf
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_particleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(FluidParticle) * m_particles.size(), m_particles.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Grab transform data and upload uniforms
    auto* pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();
    s_pParticleShader->SetUniform("model", pTransform ? pTransform->GetGlobalMatrix() : glm::mat4(1.0f));
    s_pParticleShader->SetUniform("kernelRadius", m_kernelRadius);
    s_pParticleShader->SetUniform("gasConstant", m_gasConstant);
    s_pParticleShader->SetUniform("restDensity", m_restDensity);
    s_pParticleShader->SetUniform("fluidColor", m_fluidColor);
    s_pParticleShader->SetUniform("waveColor", m_waveColor);
    s_pParticleShader->SetUniform("time", m_simTime);
    s_pParticleShader->SetUniform("causticColor", m_causticColor);
    s_pParticleShader->SetUniform("causticFrequency", m_causticFrequency);
    s_pParticleShader->Bind();

    // Bind and draw particles to our framebuffer
    GLint currentDrawFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentDrawFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(s_quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_particles.size());

    // Then unbind and blend into the previous framebuffer
    s_pBlendPassShader->Bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTextureUnit(9, s_fbColorTex);
    glBindFramebuffer(GL_FRAMEBUFFER, currentDrawFBO);
    glBindVertexArray(s_dummyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_BLEND);

    // NOTE: This is a workaround for a potential driver bug(?) in Mesa 24.2.8-1ubuntu1~24.04.1
    // Normally a texture barrier should be enough to do read-after-writes on framebuffer attachments,
    // but I experience frequent freezing on iris XE graphics without doing a full manual sync here,
    // and every other machine / OS combo I have tested does not have the same issue...
    // TODO: Investigate more if we have time
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLenum result;
    do { result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1); }
    while (result == GL_TIMEOUT_EXPIRED);
}

void BoundedFluidSystem2D::ApplyRadialForce(const glm::vec2& position, float radius, float strength)
{
    float totalRadius = radius + m_kernelRadius;
    for (auto& p : m_particles)
    {
        const float distance = glm::distance(p.m_pos, position);
        const glm::vec2 direction = glm::normalize(p.m_pos - position);
        if (distance < totalRadius)
        {
            p.m_vel += direction * glm::mix(strength, 0.0f, distance / totalRadius) / p.m_density;
        }
    }
}

void BoundedFluidSystem2D::Clear()
{
    m_particles.clear();
}

void BoundedFluidSystem2D::SpawnParticle(const glm::vec2& pos, const glm::vec2& vel)
{
    FluidParticle p(pos);
    p.m_vel = vel;
    m_particles.push_back(p);
}

void BoundedFluidSystem2D::AddStaticCollisionRect(const wolf::Rectangle& rect)
{
    m_collisionRects.push_back(rect);
}

void BoundedFluidSystem2D::AddTimedSpout(const glm::vec2& position, float lifespan, int particlesPerSecond)
{
    Spout spout;
    spout.m_pos = position;
    spout.m_lifespan = lifespan;
    spout.m_spawnRate = 1.0f / particlesPerSecond;
    m_spouts.push_back(spout);
}

void BoundedFluidSystem2D::SetParticleRadius(float radius)
{
    m_kernelRadius = radius;

    // Update dependencies
    m_kernelRadiusSqr = m_kernelRadius * m_kernelRadius;
    m_poly6 = 4.0f / (M_PI * pow(m_kernelRadius, 8.0f));
    m_spikyGradient = -10.0f / (M_PI * pow(m_kernelRadius, 5.0f));
    m_viscLaplacian = 40.0f / (M_PI * pow(m_kernelRadius, 5.0f));
    m_boundEpsilon = m_kernelRadius / 2;
}

void BoundedFluidSystem2D::ShowEditor()
{
    ImGui::Begin("##FluidSystemEditor", nullptr);
    
    ImGui::SeparatorText("Solver Parameters");
    ImGui::SliderFloat("Rest Density", &m_restDensity, 0.0f, 500.0f, "%.0f");
    ImGui::SliderFloat("Gas Constant", &m_gasConstant, 1000.0f, 3000.0f, "%.0f");
    if (ImGui::SliderFloat("Kernel Radius", &m_kernelRadius, 2.0f, 512.0f, "%.0f"))
    {
        // Update dependencies
        m_kernelRadiusSqr = m_kernelRadius * m_kernelRadius;
        m_poly6 = 4.0f / (M_PI * pow(m_kernelRadius, 8.0f));
        m_spikyGradient = -10.0f / (M_PI * pow(m_kernelRadius, 5.0f));
        m_viscLaplacian = 40.0f / (M_PI * pow(m_kernelRadius, 5.0f));
        m_boundEpsilon = m_kernelRadius / 2;
    };
    ImGui::SliderFloat("Particle Mass", &m_particleMass, 0.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Viscosity", &m_viscosity, 0.0f, 300.0f, "%.0f");
    ImGui::SliderFloat("Fixed Delta", &m_fixedDelta, 0.0001f, 0.01f, "%.4f");
    ImGui::SliderFloat("Bound Epsilon", &m_boundEpsilon, 2.f, 512.0f, "%.0f");
    ImGui::SliderFloat("Bound Damping", &m_boundDamping, -2.0f, 2.0f, "%.1f");

    ImGui::SeparatorText("Simulation Controls");
    ImGui::Checkbox("Gravity", &m_simulateGravity);
    ImGui::ColorEdit4("Fluid Color", &m_fluidColor.r);
    ImGui::ColorEdit4("Wave Color", &m_waveColor.r);
    ImGui::ColorEdit4("Caustic Color", &m_causticColor.r);
    ImGui::SliderFloat("Caustic Freq", &m_causticFrequency, 0.0f, 1.0f);
    ImGui::SliderInt("# Particles", &m_numParticlesToSpawn, 1, 5000);
    if (ImGui::Button("Respawn")) SetupDamBreak();

    ImGui::End();
}

void BoundedFluidSystem2D::SetupDamBreak()
{
    m_particles.clear();
    for (float y = m_bounds.m_top - m_kernelRadius; y > m_bounds.m_bottom + m_kernelRadius; y -= m_kernelRadius + 1)
    {
        for (float x = m_bounds.m_left + m_kernelRadius; x < m_bounds.m_right - m_kernelRadius; x += m_kernelRadius + 1)
        {
            if (m_particles.size() < m_numParticlesToSpawn)
            {
                const glm::vec2 spawnPos = glm::vec2(x + m_rng.NextFloat(-0.01f, 0.01f), y + m_rng.NextFloat(-0.01f, 0.01f));
                m_particles.emplace_back(FluidParticle(spawnPos));
            }
            else
            {
                // Done spawning particles
                return;
            }
        }
    }
}

glm::ivec2 BoundedFluidSystem2D::GetGridCell(const glm::vec2& pos)
{
    return glm::clamp(
        glm::ivec2(pos / (m_kernelRadius * 2.0f)),
        glm::ivec2(glm::vec2(m_bounds.m_left, m_bounds.m_bottom) / (m_kernelRadius * 2.0f)),
        glm::ivec2(glm::vec2(m_bounds.m_right - 1, m_bounds.m_top - 1) / (m_kernelRadius * 2.0f))
    );
}

void BoundedFluidSystem2D::ResizeFramebuffer(int width, int height)
{
    // Ensure valid input
    assert(width > 0 && height > 0);

    // Don't bother if no fluid system components exist
    if (s_refCount < 1) return;

    // Create new texture
    glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
    GLuint newTexture;
    glGenTextures(1, &newTexture);
    glBindTexture(GL_TEXTURE_2D, newTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newTexture, 0);

    // Ensure completeness
    if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        wolf::Error("Fluid sim framebuffer not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Delete old texture
    glDeleteTextures(1, &s_fbColorTex);

    // Update our handle
    s_fbColorTex = newTexture;
}