#include "ParticleModifier.h"

void EmissionShapeModifier::OnEmit(Particle& particle)
{
    if (!m_enabled) return;
    
    // Get base position and direction from particle
    glm::vec2 basePos = particle.m_pos;
    glm::vec2 baseDir = glm::normalize(particle.m_vel);
    
    // Apply emission shape
    switch (m_shapeType)
    {
        case POINT:
            EmitFromPoint(particle);
            break;
        case LINE:
            EmitFromLine(particle);
            break;
        case CIRCLE:
            EmitFromCircle(particle);
            break;
        case RECTANGLE:
            EmitFromRectangle(particle);
            break;
        case RING:
            EmitFromRing(particle);
            break;
    }
    
    // Adjust direction based on shape and settings
    if (m_randomDirection)
    {
        // Generate random angle
        std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
        float angle = glm::radians(angleDist(m_rng));
        
        // Convert to direction vector
        glm::vec2 dir(std::cos(angle), std::sin(angle));
        
        // Set velocity direction but preserve magnitude
        float speed = glm::length(particle.m_vel);
        if (speed > 0.001f)
        {
            particle.m_vel = dir * speed;
        }
        else
        {
            particle.m_vel = dir;
        }
    }
    else if (m_shapeType != POINT)
    {
        // Calculate direction based on shape and position
        glm::vec2 dir = CalculateDirection(particle.m_pos, basePos);
        
        // Apply direction angle
        float angle = glm::radians(m_directionAngle);
        dir = glm::rotate(dir, angle);
        
        // Apply spread
        if (m_spreadAngle > 0.0f)
        {
            std::uniform_real_distribution<float> spreadDist(-m_spreadAngle/2, m_spreadAngle/2);
            float spreadAngle = glm::radians(spreadDist(m_rng));
            dir = glm::rotate(dir, spreadAngle);
        }
        
        // Set velocity direction but preserve magnitude
        float speed = glm::length(particle.m_vel);
        if (speed > 0.001f)
        {
            particle.m_vel = dir * speed;
        }
        else
        {
            particle.m_vel = dir;
        }
    }
}

void EmissionShapeModifier::EmitFromPoint(Particle& particle)
{
    // No position change for point emission
}

void EmissionShapeModifier::EmitFromLine(Particle& particle)
{
    // Generate random position along line
    std::uniform_real_distribution<float> posDist(-m_width/2, m_width/2);
    float offset = posDist(m_rng);
    
    // Apply offset perpendicular to velocity
    glm::vec2 dir = glm::normalize(particle.m_vel);
    glm::vec2 perpDir(-dir.y, dir.x);
    
    particle.m_pos += perpDir * offset;
}

void EmissionShapeModifier::EmitFromCircle(Particle& particle)
{
    if (m_emitFromEdge)
    {
        // Generate random angle
        std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
        float angle = glm::radians(angleDist(m_rng));
        
        // Position on circle edge
        particle.m_pos.x += m_radius * std::cos(angle);
        particle.m_pos.y += m_radius * std::sin(angle);
    }
    else
    {
        // Generate random angle and radius
        std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
        std::uniform_real_distribution<float> radiusDist(0.0f, m_radius);
        
        float angle = glm::radians(angleDist(m_rng));
        float radius = radiusDist(m_rng);
        
        // Position within circle
        particle.m_pos.x += radius * std::cos(angle);
        particle.m_pos.y += radius * std::sin(angle);
    }
}

void EmissionShapeModifier::EmitFromRectangle(Particle& particle)
{
    if (m_emitFromEdge)
    {
        // Decide which edge to use
        std::uniform_int_distribution<int> edgeDist(0, 3);
        int edge = edgeDist(m_rng);
        
        switch (edge)
        {
            case 0: // Top
                {
                    std::uniform_real_distribution<float> xDist(-m_width/2, m_width/2);
                    particle.m_pos.x += xDist(m_rng);
                    particle.m_pos.y += m_height/2;
                }
                break;
            case 1: // Right
                {
                    std::uniform_real_distribution<float> yDist(-m_height/2, m_height/2);
                    particle.m_pos.x += m_width/2;
                    particle.m_pos.y += yDist(m_rng);
                }
                break;
            case 2: // Bottom
                {
                    std::uniform_real_distribution<float> xDist(-m_width/2, m_width/2);
                    particle.m_pos.x += xDist(m_rng);
                    particle.m_pos.y -= m_height/2;
                }
                break;
            case 3: // Left
                {
                    std::uniform_real_distribution<float> yDist(-m_height/2, m_height/2);
                    particle.m_pos.x -= m_width/2;
                    particle.m_pos.y += yDist(m_rng);
                }
                break;
        }
    }
    else
    {
        // Generate random position within rectangle
        std::uniform_real_distribution<float> xDist(-m_width/2, m_width/2);
        std::uniform_real_distribution<float> yDist(-m_height/2, m_height/2);
        
        particle.m_pos.x += xDist(m_rng);
        particle.m_pos.y += yDist(m_rng);
    }
}

void EmissionShapeModifier::EmitFromRing(Particle& particle)
{
    // Generate random angle
    std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
    float angle = glm::radians(angleDist(m_rng));
    
    // Generate random radius within ring
    std::uniform_real_distribution<float> radiusDist(m_innerRadius, m_radius);
    float radius = radiusDist(m_rng);
    
    // Position on ring
    particle.m_pos.x += radius * std::cos(angle);
    particle.m_pos.y += radius * std::sin(angle);
}

glm::vec2 EmissionShapeModifier::CalculateDirection(const glm::vec2& position, const glm::vec2& center)
{
    // By default, direction is outward from center
    glm::vec2 dir = position - center;
    
    // Normalize if possible
    float length = glm::length(dir);
    if (length > 0.001f)
    {
        dir /= length;
    }
    else
    {
        // Fallback to upward direction
        dir = glm::vec2(0.0f, 1.0f);
    }
    
    return dir;
}