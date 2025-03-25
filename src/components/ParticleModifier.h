#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <vector>
#include <memory>
#include <functional>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <random>
#include <unordered_map>

// Include Particle.h to get the Particle struct definition
#include "Particle.h"

// Forward declarations
class ParticleComponent;

// Base class for all particle modifiers
class ParticleModifier
{
public:
    virtual ~ParticleModifier() = default;
    
    // Called when a particle is emitted
    virtual void OnEmit(Particle& particle) {}
    
    // Called each frame to update the particle
    virtual void Update(Particle& particle, float delta) {}
    
    // Enable/disable the modifier
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    
    // Name accessor for editor display
    const std::string& GetName() const { return m_name; }
    
protected:
    ParticleModifier(const std::string& name) : m_name(name), m_enabled(true) {}
    
    std::string m_name;
    bool m_enabled;
};

//-----------------------------------------------------------------------------
// Emission Modifiers
//-----------------------------------------------------------------------------

// Controls how particles are emitted in an area/shape
class EmissionShapeModifier : public ParticleModifier
{
public:
    enum ShapeType
    {
        POINT,
        LINE,
        CIRCLE,
        RECTANGLE,
        RING
    };
    
    EmissionShapeModifier() 
        : ParticleModifier("Emission Shape"), 
          m_shapeType(POINT),
          m_width(10.0f),
          m_height(10.0f),
          m_radius(20.0f),
          m_innerRadius(10.0f),
          m_emitFromEdge(false),
          m_randomDirection(false),
          m_directionAngle(0.0f),
          m_spreadAngle(30.0f)
    {
        // Initialize random engine
        std::random_device rd;
        m_rng = std::mt19937(rd());
    }
    
    void OnEmit(Particle& particle) override;
    
    // Configuration
    void SetShapeType(ShapeType type) { m_shapeType = type; }
    ShapeType GetShapeType() const { return m_shapeType; }
    
    void SetSize(float width, float height) { m_width = width; m_height = height; }
    void SetRadius(float radius) { m_radius = radius; }
    void SetInnerRadius(float innerRadius) { m_innerRadius = innerRadius; }
    
    void SetEmitFromEdge(bool fromEdge) { m_emitFromEdge = fromEdge; }
    bool GetEmitFromEdge() const { return m_emitFromEdge; }
    
    void SetRandomDirection(bool randomDir) { m_randomDirection = randomDir; }
    bool GetRandomDirection() const { return m_randomDirection; }
    
    void SetDirectionAngle(float angleDegrees) { m_directionAngle = angleDegrees; }
    float GetDirectionAngle() const { return m_directionAngle; }
    
    void SetSpreadAngle(float angleDegrees) { m_spreadAngle = angleDegrees; }
    float GetSpreadAngle() const { return m_spreadAngle; }
    
private:
    ShapeType m_shapeType;
    float m_width, m_height;   // For LINE and RECTANGLE
    float m_radius;            // For CIRCLE and RING
    float m_innerRadius;       // For RING
    bool m_emitFromEdge;       // For CIRCLE/RECTANGLE - emit from edge only
    
    bool m_randomDirection;    // Random direction override
    float m_directionAngle;    // Direction angle in degrees (0 = right, 90 = up)
    float m_spreadAngle;       // Spread angle for randomness in degrees
    
    std::mt19937 m_rng;
    
    // Helper methods for different shapes
    void EmitFromPoint(Particle& particle);
    void EmitFromLine(Particle& particle);
    void EmitFromCircle(Particle& particle);
    void EmitFromRectangle(Particle& particle);
    void EmitFromRing(Particle& particle);
    
    // Direction calculation
    glm::vec2 CalculateDirection(const glm::vec2& position, const glm::vec2& center);
};

//-----------------------------------------------------------------------------
// Particle Behavior Modifiers
//-----------------------------------------------------------------------------

// Adds gravity effect to particles
class GravityModifier : public ParticleModifier
{
public:
    GravityModifier() 
        : ParticleModifier("Gravity"),
          m_gravity(0.0f, 9.8f),
          m_strength(1.0f)
    {}
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Apply gravity force
        particle.m_vel += m_gravity * m_strength * delta;
    }
    
    void SetGravity(const glm::vec2& gravity) { m_gravity = gravity; }
    const glm::vec2& GetGravity() const { return m_gravity; }
    
    void SetStrength(float strength) { m_strength = strength; }
    float GetStrength() const { return m_strength; }
    
private:
    glm::vec2 m_gravity;  // Direction and magnitude of gravity
    float m_strength;     // Multiplier for gravity effect
};

// Adds drag/air resistance to slow particles over time
class DragModifier : public ParticleModifier
{
public:
    DragModifier() 
        : ParticleModifier("Drag"),
          m_dragCoefficient(0.1f)
    {}
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Apply drag force: F = -k * v
        float speed = glm::length(particle.m_vel);
        if (speed > 0.001f)
        {
            float dragFactor = 1.0f - (m_dragCoefficient * delta);
            dragFactor = glm::max(0.0f, dragFactor); // Prevent negative values
            particle.m_vel *= dragFactor;
        }
    }
    
    void SetDragCoefficient(float drag) { m_dragCoefficient = drag; }
    float GetDragCoefficient() const { return m_dragCoefficient; }
    
private:
    float m_dragCoefficient;  // Higher values mean more drag
};

// Makes particles orbit around a point
class VortexModifier : public ParticleModifier
{
public:
    VortexModifier() 
        : ParticleModifier("Vortex"),
          m_centerOffset(0.0f, 0.0f),
          m_strength(1.0f),
          m_rotationSpeed(90.0f), // Degrees per second
          m_attractionStrength(0.5f)
    {}
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Calculate direction to center
        glm::vec2 toCenter = m_centerOffset - particle.m_pos;
        float distance = glm::length(toCenter);
        
        if (distance < 0.001f) return; // Avoid division by zero
        
        // Normalize
        toCenter /= distance;
        
        // Calculate tangent direction (perpendicular to radial)
        glm::vec2 tangent(-toCenter.y, toCenter.x);
        
        // Apply vortex force - tangential component for rotation
        float rotationForce = m_rotationSpeed * delta * m_strength;
        particle.m_vel += tangent * rotationForce;
        
        // Apply attraction force - radial component
        float attractionForce = m_attractionStrength * delta * m_strength;
        particle.m_vel += toCenter * attractionForce;
    }
    
    void SetCenterOffset(const glm::vec2& offset) { m_centerOffset = offset; }
    const glm::vec2& GetCenterOffset() const { return m_centerOffset; }
    
    void SetStrength(float strength) { m_strength = strength; }
    float GetStrength() const { return m_strength; }
    
    void SetRotationSpeed(float degreesPerSec) { m_rotationSpeed = degreesPerSec; }
    float GetRotationSpeed() const { return m_rotationSpeed; }
    
    void SetAttractionStrength(float strength) { m_attractionStrength = strength; }
    float GetAttractionStrength() const { return m_attractionStrength; }
    
private:
    glm::vec2 m_centerOffset;  // Center of vortex relative to emitter
    float m_strength;          // Overall strength multiplier
    float m_rotationSpeed;     // How fast particles rotate around center
    float m_attractionStrength; // How strongly particles are pulled to center
};

// Makes particles scatter or converge toward a point
class AttractorModifier : public ParticleModifier
{
public:
    AttractorModifier() 
        : ParticleModifier("Attractor"),
          m_attractorOffset(0.0f, 0.0f),
          m_strength(1.0f),
          m_falloffRadius(100.0f),
          m_isRepeller(false)
    {}
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Calculate direction to attractor
        glm::vec2 toAttractor = m_attractorOffset - particle.m_pos;
        float distance = glm::length(toAttractor);
        
        if (distance < 0.001f) return; // Avoid division by zero
        
        // Normalize
        toAttractor /= distance;
        
        // Calculate falloff based on distance
        float falloff = 1.0f;
        if (m_falloffRadius > 0.0f)
        {
            falloff = glm::max(0.0f, 1.0f - (distance / m_falloffRadius));
        }
        
        // Apply force toward (or away from) attractor
        float force = m_strength * falloff * delta;
        if (m_isRepeller)
            force = -force;
            
        particle.m_vel += toAttractor * force;
    }
    
    void SetAttractorOffset(const glm::vec2& offset) { m_attractorOffset = offset; }
    const glm::vec2& GetAttractorOffset() const { return m_attractorOffset; }
    
    void SetStrength(float strength) { m_strength = strength; }
    float GetStrength() const { return m_strength; }
    
    void SetFalloffRadius(float radius) { m_falloffRadius = radius; }
    float GetFalloffRadius() const { return m_falloffRadius; }
    
    void SetIsRepeller(bool isRepeller) { m_isRepeller = isRepeller; }
    bool GetIsRepeller() const { return m_isRepeller; }
    
private:
    glm::vec2 m_attractorOffset;  // Attractor position relative to emitter
    float m_strength;            // Attraction force strength
    float m_falloffRadius;       // Distance at which the effect falls to zero
    bool m_isRepeller;           // If true, particles are pushed away instead
};

//-----------------------------------------------------------------------------
// Appearance Modifiers
//-----------------------------------------------------------------------------

// Changes particle size over lifetime
class SizeOverLifetimeModifier : public ParticleModifier
{
public:
    SizeOverLifetimeModifier() 
        : ParticleModifier("Size Over Lifetime"),
          m_startScale(1.0f),
          m_endScale(0.5f),
          m_curve(EASE_OUT)
    {}
    
    enum CurveType
    {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT
    };
    
    void OnEmit(Particle& particle) override
    {
        if (!m_enabled) return;
        
        // Store original size for reference
        m_originalSizes[&particle] = particle.m_size;
    }
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Find original size or use current
        float originalSize = particle.m_size;
        auto it = m_originalSizes.find(&particle);
        if (it != m_originalSizes.end())
        {
            originalSize = it->second;
        }
        else
        {
            // Store it for future reference
            m_originalSizes[&particle] = originalSize;
        }
        
        // Calculate life progress (1.0 = new, 0.0 = dead)
        float t = particle.m_lifetime / particle.m_initialLifetime;
        
        // Apply curve function
        float curvedT = ApplyCurve(1.0f - t);  // Invert t since we're counting down
        
        // Interpolate between start and end scale
        float scale = m_startScale + (m_endScale - m_startScale) * curvedT;
        
        // Apply scale to original size
        particle.m_size = originalSize * scale;
    }
    
    void SetScales(float start, float end)
    {
        m_startScale = start;
        m_endScale = end;
    }
    
    void SetCurveType(CurveType curve) { m_curve = curve; }
    CurveType GetCurveType() const { return m_curve; }
    
    float GetStartScale() const { return m_startScale; }
    float GetEndScale() const { return m_endScale; }
    
private:
    float m_startScale;
    float m_endScale;
    CurveType m_curve;
    
    // Store original sizes of particles
    std::unordered_map<Particle*, float> m_originalSizes;
    
    // Apply easing curve to t (0-1)
    float ApplyCurve(float t) const
    {
        switch (m_curve)
        {
            case LINEAR:
                return t;
            case EASE_IN:
                return t * t;
            case EASE_OUT:
                return 1.0f - (1.0f - t) * (1.0f - t);
            case EASE_IN_OUT:
                return t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
            default:
                return t;
        }
    }
};

// Changes particle color over lifetime
class ColorOverLifetimeModifier : public ParticleModifier
{
public:
    ColorOverLifetimeModifier() 
        : ParticleModifier("Color Over Lifetime"),
          m_startColor(1.0f, 1.0f, 1.0f, 1.0f),
          m_endColor(1.0f, 1.0f, 1.0f, 0.0f)  // Default to fade out
    {}
    
    void OnEmit(Particle& particle) override
    {
        if (!m_enabled) return;
        
        // Store original color for reference
        m_originalColors[&particle] = particle.m_color;
    }
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Find original color or use current
        glm::vec4 originalColor = particle.m_color;
        auto it = m_originalColors.find(&particle);
        if (it != m_originalColors.end())
        {
            originalColor = it->second;
        }
        else
        {
            // Store it for future reference
            m_originalColors[&particle] = originalColor;
        }
        
        // Calculate life progress (1.0 = new, 0.0 = dead)
        float t = particle.m_lifetime / particle.m_initialLifetime;
        
        // Interpolate between colors
        glm::vec4 targetColor = m_startColor * t + m_endColor * (1.0f - t);
        
        // Apply color, preserving original color as a base
        particle.m_color = glm::vec4(
            originalColor.r * targetColor.r,
            originalColor.g * targetColor.g,
            originalColor.b * targetColor.b,
            originalColor.a * targetColor.a
        );
    }
    
    void SetColors(const glm::vec4& start, const glm::vec4& end)
    {
        m_startColor = start;
        m_endColor = end;
    }
    
    const glm::vec4& GetStartColor() const { return m_startColor; }
    const glm::vec4& GetEndColor() const { return m_endColor; }
    
private:
    glm::vec4 m_startColor;
    glm::vec4 m_endColor;
    
    // Store original colors of particles
    std::unordered_map<Particle*, glm::vec4> m_originalColors;
};

// Makes particles rotate over time
class RotationModifier : public ParticleModifier
{
public:
    RotationModifier() 
        : ParticleModifier("Rotation"),
          m_rotationSpeed(90.0f),  // Degrees per second
          m_randomizeInitialRotation(true),
          m_randomizeRotationDirection(true)
    {
        // Initialize random engine
        std::random_device rd;
        m_rng = std::mt19937(rd());
    }
    
    void OnEmit(Particle& particle) override
    {
        if (!m_enabled) return;
        
        // Initialize rotation data for this particle
        RotationData data;
        
        // Random initial rotation
        if (m_randomizeInitialRotation)
        {
            std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
            data.currentRotation = rotDist(m_rng);
        }
        
        // Random direction
        if (m_randomizeRotationDirection)
        {
            std::bernoulli_distribution dirDist(0.5);
            data.direction = dirDist(m_rng) ? 1.0f : -1.0f;
        }
        
        // Store data
        m_rotationData[&particle] = data;
    }
    
    void Update(Particle& particle, float delta) override
    {
        if (!m_enabled) return;
        
        // Get or create rotation data
        auto it = m_rotationData.find(&particle);
        if (it == m_rotationData.end())
        {
            OnEmit(particle);
            it = m_rotationData.find(&particle);
        }
        
        // Update rotation
        RotationData& data = it->second;
        data.currentRotation += m_rotationSpeed * data.direction * delta;
        
        // Keep rotation in 0-360 range
        while (data.currentRotation >= 360.0f)
            data.currentRotation -= 360.0f;
        while (data.currentRotation < 0.0f)
            data.currentRotation += 360.0f;
        
        // Apply rotation to particle
        particle.m_rotation = data.currentRotation;
        particle.m_angularVelocity = m_rotationSpeed * data.direction;
    }
    
    void SetRotationSpeed(float degreesPerSec) { m_rotationSpeed = degreesPerSec; }
    float GetRotationSpeed() const { return m_rotationSpeed; }
    
    void SetRandomizeInitialRotation(bool randomize) { m_randomizeInitialRotation = randomize; }
    bool GetRandomizeInitialRotation() const { return m_randomizeInitialRotation; }
    
    void SetRandomizeRotationDirection(bool randomize) { m_randomizeRotationDirection = randomize; }
    bool GetRandomizeRotationDirection() const { return m_randomizeRotationDirection; }
    
private:
    struct RotationData
    {
        float currentRotation = 0.0f;  // Current rotation in degrees
        float direction = 1.0f;        // 1 or -1 for clockwise/counterclockwise
    };
    
    float m_rotationSpeed;
    bool m_randomizeInitialRotation;
    bool m_randomizeRotationDirection;
    
    std::mt19937 m_rng;
    std::unordered_map<Particle*, RotationData> m_rotationData;
};