#include "BoundedFluidSystem2D.h"

#include <glm/gtx/norm.hpp>

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
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(FluidParticle) * 500, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Create shader
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/fluid_particle.vs", "data/shaders/fluid_particle.fs");
    }
    s_refCount++;

    // DEBUG: Initial dam break configuration for testing
    int numParticles = 500;
    for (float y = m_bounds.m_top; y > m_bounds.m_bottom; y -= KERNEL_RADIUS + 1)
    {
        for (float x = m_bounds.m_left; x < m_bounds.m_right; x += KERNEL_RADIUS + 1)
        {
            if (m_particles.size() < numParticles)
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
            if (sqrDist < KERNEL_RADIUS_SQR)
            {
                particle.m_density += PARTICLE_MASS * POLY6 * pow(KERNEL_RADIUS_SQR - sqrDist, 3.0f);
            }
        }

        // Calculate pressure
        particle.m_pressure = GAS_CONSTANT * (particle.m_density - REST_DENSITY);
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

            if (dist < KERNEL_RADIUS)
            {
                pressureForce += -glm::normalize(between) * PARTICLE_MASS * (particle.m_pressure + other.m_pressure) / (2.0f * other.m_density) * SPIKY_GRAD * (float)pow(KERNEL_RADIUS - dist, 3.0f);
                viscosityForce += VISCOSITY * PARTICLE_MASS * (other.m_vel - particle.m_vel) / other.m_density * VISC_LAP * (KERNEL_RADIUS - dist);
            }
        }

        // Calculate force of gravity and add final forces together
        glm::vec2 gravityForce = GRAVITY * PARTICLE_MASS / particle.m_density;
        particle.m_force = pressureForce + viscosityForce + gravityForce;
    }

    Integrate();
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
    s_pShader->SetUniform("kernelRadius", KERNEL_RADIUS);
    s_pShader->Bind();
    glBindVertexArray(s_quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_particles.size());
    glBindVertexArray(0);
}

void BoundedFluidSystem2D::Integrate()
{
    for (auto& p : m_particles)
    {
        // Forward Euler integration
        p.m_vel += FIXED_DELTA * p.m_force / p.m_density;
        p.m_pos += FIXED_DELTA * p.m_vel;
        // p.m_vel *= 0.95f; // Extra damping to help with stability

        // Boundary enforcement
        if (p.m_pos.x - BOUND_EPSILON < m_bounds.m_left)
        {
            p.m_vel.x *= BOUND_DAMPING;
            p.m_pos.x = m_bounds.m_left + BOUND_EPSILON;
        }
        if (p.m_pos.x + BOUND_EPSILON > m_bounds.m_right)
        {
            p.m_vel.x *= BOUND_DAMPING;
            p.m_pos.x = m_bounds.m_right - BOUND_EPSILON;
        }
        if (p.m_pos.y - BOUND_EPSILON < m_bounds.m_bottom)
        {
            p.m_vel.y *= BOUND_DAMPING;
            p.m_pos.y = m_bounds.m_bottom + BOUND_EPSILON;
        }
        if (p.m_pos.y + BOUND_EPSILON > m_bounds.m_top)
        {
            p.m_vel.y *= BOUND_DAMPING;
            p.m_pos.y = m_bounds.m_top - BOUND_EPSILON;
        }
    }
}

void BoundedFluidSystem2D::ApplyRadialForce(const glm::vec2& position, float radius, float strength)
{
    // TODO: This will benefit greatly from the spatial hashing optimizations...
    float totalRadius = radius + KERNEL_RADIUS;
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