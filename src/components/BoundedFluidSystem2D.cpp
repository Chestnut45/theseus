#include "BoundedFluidSystem2D.h"

#include <glm/gtx/norm.hpp>
#include <imgui/imgui.h>

BoundedFluidSystem2D::BoundedFluidSystem2D(const wolf::Rectangle& bounds)
    :
    m_bounds(bounds)
{
    // Reference counting
    if (s_refCount == 0)
    {
        // Initialize static resources

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

        // Create shader
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/fluid_particle.vs", "data/shaders/fluid_particle.fs");
    }
    s_refCount++;

    SetupDamBreak();
}

BoundedFluidSystem2D::~BoundedFluidSystem2D()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        // Cleanup static resources
        glDeleteBuffers(1, &s_quadVBO);
        glDeleteBuffers(1, &s_particleSSBO);
        glDeleteVertexArrays(1, &s_quadVAO);
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }
}

void BoundedFluidSystem2D::Update(float delta)
{
    // Compute the density and pressure of each particle
    for (auto& particle : m_particles)
    {
        particle.m_density = 0.0f;

        // Sum density contributions from other particles
        for (auto& other : m_particles)
        {
            glm::vec2 between = other.m_pos - particle.m_pos;
            float sqrDist = glm::length2(between);
            if (sqrDist < m_kernelRadiusSqr)
            {
                particle.m_density += m_particleMass * m_poly6 * pow(m_kernelRadiusSqr - sqrDist, 3.0f);
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
        
        for (auto& other : m_particles)
        {
            if (&particle == &other) continue;

            glm::vec2 between = other.m_pos - particle.m_pos;
            float dist = glm::length(between);

            if (dist < m_kernelRadius)
            {
                pressureForce += -glm::normalize(between) * m_particleMass * (particle.m_pressure + other.m_pressure) / (2.0f * other.m_density) * m_spikyGradient * (float)pow(m_kernelRadius - dist, 3.0f);
                viscosityForce += m_viscosity * m_particleMass * (other.m_vel - particle.m_vel) / other.m_density * m_viscLaplacian * (m_kernelRadius - dist);
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

        // p.m_vel *= 0.95f; // DEBUG: Extra damping to help with stability

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
    // TODO: Even if not, moving the upload to immediately after update may help
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_particleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(FluidParticle) * m_particles.size(), m_particles.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // TODO: Setup blend state

    // TODO: Transform uniform data

    // Bind and draw
    s_pShader->SetUniform("kernelRadius", m_kernelRadius);
    s_pShader->Bind();
    glBindVertexArray(s_quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_particles.size());
    glBindVertexArray(0);
}

void BoundedFluidSystem2D::ApplyRadialForce(const glm::vec2& position, float radius, float strength)
{
    // TODO: This will benefit greatly from the spatial hashing optimizations...
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

void BoundedFluidSystem2D::ShowEditor()
{
    ImGui::Begin("##FluidSystemEditor", nullptr);
    ImGui::Checkbox("Gravity", &m_simulateGravity);
    ImGui::SliderFloat("Rest Density", &m_restDensity, 0.0f, 500.0f, "%.0f");
    ImGui::SliderFloat("Gas Constant", &m_gasConstant, 1000.0f, 3000.0f, "%.0f");
    if (ImGui::SliderFloat("Kernel Radius", &m_kernelRadius, 2.0f, 512.0f, "%.0f"))
    {
        // Update dependencies
        m_kernelRadiusSqr = m_kernelRadius * m_kernelRadius;
        m_poly6 = 4.0f / (M_PI * pow(m_kernelRadius, 8.0f));
        m_spikyGradient = -10.0f / (M_PI * pow(m_kernelRadius, 5.0f));
        m_viscLaplacian = 40.0f / (M_PI * pow(m_kernelRadius, 5.0f));
        m_boundEpsilon = m_kernelRadius;
    };
    ImGui::SliderFloat("Particle Mass", &m_particleMass, 0.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Viscosity", &m_viscosity, 0.0f, 300.0f, "%.0f");
    ImGui::SliderFloat("Fixed Delta", &m_fixedDelta, 0.0001f, 0.01f, "%.4f");
    ImGui::SliderFloat("Bound Epsilon", &m_boundEpsilon, 2.f, 512.0f, "%.0f");
    ImGui::SliderFloat("Bound Damping", &m_boundDamping, -2.0f, 2.0f, "%.1f");
    ImGui::SliderInt("#Particles", &m_numParticlesToSpawn, 1, 5000);
    if (ImGui::Button("Respawn")) SetupDamBreak();

    ImGui::End();
}

void BoundedFluidSystem2D::SetupDamBreak()
{
    m_particles.clear();
    for (float y = m_bounds.m_top; y > m_bounds.m_bottom; y -= m_kernelRadius + 1)
    {
        for (float x = m_bounds.m_left; x < m_bounds.m_right; x += m_kernelRadius + 1)
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