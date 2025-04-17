#pragma once

//-----------------------------------------------------------------------------
// File:			ParticleComponent.h
// Original Author:	Youssef Ashraf
// ver 1.3
// Class responsible for particles
//-----------------------------------------------------------------------------


#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <GL/glew.h>
#include <W_BaseComponent.h>
#include <string>
#include "W_Program.h"
#include <W_Texture.h>
#include <W_TextureManager.h>
#include <memory>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "Particle.h"

// Forward declaration for modifier classes
class ParticleModifier;

class ParticleComponent : public wolf::BaseComponent
{
public:
    ParticleComponent(size_t maxParticles = 100);
    ~ParticleComponent();

    ParticleComponent(const ParticleComponent& other) = delete;
    ParticleComponent& operator=(const ParticleComponent& other) = delete;

    ParticleComponent(ParticleComponent&& other) = delete;
    ParticleComponent& operator=(ParticleComponent&& other) = delete;

    void Update(float delta);
    void Render();
    
    // Enhanced emit method with more options
    void Emit(const glm::vec2& position, const glm::vec2& velocity, const glm::vec4& color, 
              float size, float lifetime, wolf::Texture* texture = nullptr,
              float rotation = 0.0f, float angularVelocity = 0.0f);
    
    // Burst emission helper
    void EmitBurst(const glm::vec2& position, const glm::vec2& baseVelocity, const glm::vec4& color,
                  float size, float lifetime, int count, float spread = 360.0f, 
                  wolf::Texture* texture = nullptr);

    // Original accessors
    std::vector<Particle>& GetParticles() { return m_particles; }
    void SetMaxParticles(size_t maxParticles);
    size_t GetMaxParticles() const { return m_particles.size(); }
    
    // Enhanced modifiers system
    void AddModifier(std::shared_ptr<ParticleModifier> modifier);
    void RemoveModifier(const std::string& modifierName);
    std::shared_ptr<ParticleModifier> GetModifier(const std::string& modifierName);

    // load configurations from YAML files
    bool LoadConfigFromYAML(const std::string& filename);

    // Auto-cleanup settings
    void SetAutoDestroy(bool autoDestroy) { m_autoDestroy = autoDestroy; }
    bool GetAutoDestroy() const { return m_autoDestroy; }
    void SetCleanupGracePeriod(float seconds) { m_cleanupGracePeriod = seconds; }
    float GetCleanupGracePeriod() const { return m_cleanupGracePeriod; }

    void SetContinuousEmission(bool enable) { m_continuousEmission = enable; }
    bool GetContinuousEmission() const { return m_continuousEmission; }
    
    // Emission rate (particles per second)
    void SetEmissionRate(float rate) { m_emissionRate = rate; }
    float GetEmissionRate() const { return m_emissionRate; }
    
    // Emission direction
    void SetEmissionDirection(const glm::vec2& direction) { 
        m_emissionDirection = glm::normalize(direction); 
    }
    glm::vec2 GetEmissionDirection() const { return m_emissionDirection; }
    
    // Base velocity for particles
    void SetBaseVelocity(const glm::vec2& velocity) { m_baseVelocity = velocity; }
    glm::vec2 GetBaseVelocity() const { return m_baseVelocity; }
    
    // User data (for storing timers or other custom data)
    void SetUserData(void* data) { m_userData = data; }
    void* GetUserData() const { return m_userData; }


private:
    std::vector<Particle> m_particles;
    std::vector<std::shared_ptr<ParticleModifier>> m_modifiers;

    GLuint m_vao, m_posVBO, m_colorVBO, m_sizeVBO;
    GLuint m_quadVAO, m_quadVBO, m_quadEBO;
    GLuint m_rotationVBO; // New VBO for rotation

    // Auto-cleanup functionality
    bool m_autoDestroy = false;       // Whether to auto-destroy when all particles are inactive
    float m_cleanupGracePeriod = 0.5f; // Grace period after all particles are inactive
    float m_cleanupTimer = 0.0f;     // Timer for grace period
    bool m_hasActiveParticles = false; // Track if we've had active particles

    static inline wolf::Program* s_pShader = nullptr;
    static inline size_t s_refCount = 0;

    void InitGLResources();
    void InitQuadResources();
    void CleanupGLResources();
    
    // Helper methods for rendering
    void RenderPointParticles();
    void RenderTexturedParticles();
    
    // Apply all modifiers to a particle
    void ApplyModifiers(Particle& particle, float delta, bool isNewParticle = false);

    // New properties for continuous emission
    bool m_continuousEmission = false;
    float m_emissionRate = 10.0f; // Particles per second
    float m_emissionTimer = 0.0f;
    glm::vec2 m_emissionDirection = glm::vec2(0.0f, 1.0f);
    glm::vec2 m_baseVelocity = glm::vec2(0.0f, 0.0f);
    void* m_userData = nullptr;

    wolf::Texture* m_pTex = nullptr;
    bool m_additiveBlend = false;

    // Cache of config nodes so we don't load from disk every time blood happens
    static inline std::unordered_map<std::string, YAML::Node> s_configNodeMap;
};