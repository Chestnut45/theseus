#pragma once

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

// Include the Particle struct from its own file
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


private:
    std::vector<Particle> m_particles;
    std::vector<std::shared_ptr<ParticleModifier>> m_modifiers;

    GLuint m_vao, m_posVBO, m_colorVBO, m_sizeVBO;
    GLuint m_quadVAO, m_quadVBO, m_quadEBO;
    GLuint m_rotationVBO; // New VBO for rotation

    // Auto-cleanup functionality
    bool m_autoDestroy = true;       // Whether to auto-destroy when all particles are inactive
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
};