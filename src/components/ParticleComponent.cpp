//-----------------------------------------------------------------------------
// File:			ParticleComponent.h
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for particle data, update and emission.
//-----------------------------------------------------------------------------
#include "ParticleComponent.h"
#include "iostream"

ParticleComponent::ParticleComponent(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}

void ParticleComponent::Update(float delta)
{
    int activeParticles = 0;
    for (auto& particle : m_particles)
    {
        if (particle.m_active)
        {
            particle.Update(delta);
            activeParticles++; // Count active particles
        }
    }
}


void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime)
{
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            particle.Reset(position, velocity, color, size, lifetime);
            particle.m_active = true;
            // std::cout << "Emitted a new particle at (" << position.x << ", " << position.y << ")" << std::endl;
            return;
        }
    }
    std::cout << "No available particle slots!" << std::endl;
}
