//-----------------------------------------------------------------------------
// File:			ParticleComponent.h
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for particle data, update and emission.
//-----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <W_BaseComponent.h>

struct Particle
{
    glm::vec2 m_pos;
    glm::vec2 m_vel;
    glm::vec4 m_color;
    float m_size;
    float m_lifetime;
    float m_initialLifetime;
    bool m_active = false;

    void Reset(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime)
    {
        m_pos = position;
        m_vel = velocity;
        m_color = color;
        m_size = size;
        m_lifetime = lifetime;
        m_initialLifetime = lifetime;
        m_active = true;
    }

    void Update(float delta)
    {
        if (!m_active) return;

        m_pos += m_vel * delta;
        m_lifetime -= delta;
        if (m_lifetime <= 0.0f) m_active = false;
    }
};

class ParticleComponent : public wolf::BaseComponent
{
public:
    ParticleComponent(size_t maxParticles = 100);
    
    void Update(float delta);
    void Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, float size, float lifetime);
    
    std::vector<Particle>& GetParticles() { return m_particles; }

private:
    std::vector<Particle> m_particles;
};
