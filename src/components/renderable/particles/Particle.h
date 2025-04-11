#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <W_Texture.h>

// Particle struct definition
struct Particle
{
    glm::vec2 m_pos;
    glm::vec2 m_vel;
    glm::vec4 m_color;
    float m_size;
    float m_lifetime;
    float m_initialLifetime;
    bool m_active = false;
    
    wolf::Texture* m_texture = nullptr; // Texture support for quads
    
    // New properties for enhanced particles
    float m_rotation = 0.0f;            // Rotation in degrees
    float m_angularVelocity = 0.0f;     // Angular velocity in degrees per second
    float m_startSize = 0.0f;           // Initial size (for animations)
    glm::vec4 m_startColor;             // Initial color (for animations)

    void Reset(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, 
               float size, float lifetime, wolf::Texture* texture = nullptr)
    {
        // Don't destroy textures here
        // Set pointer to null without attempting to destroy
        m_texture = texture;
        
        m_pos = position;
        m_vel = velocity;
        m_color = color;
        m_size = size;
        m_lifetime = lifetime;
        m_initialLifetime = lifetime;
        m_active = true;
        
        // Reset enhanced properties
        m_rotation = 0.0f;
        m_angularVelocity = 0.0f;
        m_startSize = size;
        m_startColor = color;
    }

    void Update(float delta)
    {
        if (!m_active) return;
        
        // Basic update
        m_pos += m_vel * delta;
        m_lifetime -= delta;
        
        // Enhanced update
        m_rotation += m_angularVelocity * delta;
        
        // Normalize rotation to 0-360 range
        m_rotation = fmodf(m_rotation, 360.0f);
        if (m_rotation < 0) m_rotation += 360.0f;
            
        if (m_lifetime <= 0.0f)
            m_active = false;
    }
};