#include "ParticleComponent.h"

ParticleComponent::ParticleComponent(size_t maxParticles)
{
    m_particles.resize(maxParticles);
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
