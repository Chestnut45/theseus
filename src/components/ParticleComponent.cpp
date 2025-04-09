#define GLM_ENABLE_EXPERIMENTAL

#include "ParticleComponent.h"
#include "ParticleModifier.h"
#include <W_ProgramManager.h>
#include <unordered_map>
#include <algorithm>
#include <W_Logging.h>
#include <glm/gtc/matrix_transform.hpp>
#include <W_GameObject.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <W_Transform2D.h>
#include <unordered_set>

ParticleComponent::ParticleComponent(size_t maxParticles)
{
    m_particles.resize(maxParticles);

    if (s_refCount == 0)
    {
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/particle2D.vs", "data/shaders/particle2D.fs");
        
        if (!s_pShader)
        {
            wolf::Error("Failed to create particle shader program!");
        }
    }
    s_refCount++;

    InitGLResources();
    InitQuadResources();
}

ParticleComponent::~ParticleComponent()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }

    // Use a set to track valid textures
    std::unordered_set<wolf::Texture*> uniqueTextures;
        
    // Collect all valid textures
    for (auto& particle : m_particles)
    {
        if (particle.m_texture != nullptr)
        {
            uniqueTextures.insert(particle.m_texture);
            particle.m_texture = nullptr; 
        }
    }

    // Destroy each texture
    for (auto* texture : uniqueTextures)
    {
        wolf::TextureManager::DestroyTexture(texture);
    }


    // Clear modifiers
    m_modifiers.clear();

    CleanupGLResources();
}

void ParticleComponent::InitGLResources()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &m_sizeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    // New rotation VBO for point particles
    glGenBuffers(1, &m_rotationVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_rotationVBO);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(4);

    glEnable(GL_PROGRAM_POINT_SIZE);
}

void ParticleComponent::InitQuadResources()
{
    // Define a unit quad with texture coordinates
    const GLfloat vertices[] = {
        // Positions  // TexCoords
        -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
         0.5f, -0.5f,  1.0f, 0.0f, // Bottom-right
         0.5f,  0.5f,  1.0f, 1.0f, // Top-right
        -0.5f,  0.5f,  0.0f, 1.0f  // Top-left
    };

    const GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

    // Single-call buffer and VAO generation
    GLuint buffers[2];
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(2, buffers);
    m_quadVBO = buffers[0];
    m_quadEBO = buffers[1];

    glBindVertexArray(m_quadVAO);

    // Vertex buffer setup
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}


void ParticleComponent::CleanupGLResources()
{
    // Consolidated buffer deletion
    GLuint particleBuffers[] = { m_posVBO, m_colorVBO, m_sizeVBO, m_rotationVBO };
    GLuint particleVAOs[] = { m_vao };
    GLuint quadBuffers[] = { m_quadVBO, m_quadEBO };
    GLuint quadVAOs[] = { m_quadVAO };

    glDeleteBuffers(4, particleBuffers);
    glDeleteVertexArrays(1, particleVAOs);

    glDeleteBuffers(2, quadBuffers);
    glDeleteVertexArrays(1, quadVAOs);
}

void ParticleComponent::Update(float delta)
{
    bool anyActive = false;
    
    for (auto& particle : m_particles)
    {
        if (particle.m_active)
        {
            // Apply modifiers first
            ApplyModifiers(particle, delta);
            
            // Then do basic update
            particle.Update(delta);
            
            // Flag that at least one particle is active
            anyActive = true;
        }
    }

    if (!m_hasActiveParticles)
    {
        m_hasActiveParticles = anyActive;
    }
    
    // Handle continuous emission if enabled
    if (m_continuousEmission)
    {
        m_emissionTimer += delta;
        float emissionInterval = 1.0f / m_emissionRate; // Time between emissions
        
        if (m_emissionTimer >= emissionInterval)
        {
            // Get the parent object's position
            glm::vec2 position = glm::vec2(0.0f);
            auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
            if (transform)
            {
                position = transform->GetGlobalPosition();
            }
            
            // Calculate velocity based on emission direction and base velocity
            glm::vec2 velocity = m_baseVelocity;
            if (glm::length(velocity) < 0.001f)
            {
                velocity = m_emissionDirection * 50.0f; // Default speed if none specified
            }
            
            // Emit a particle
            glm::vec4 color = glm::vec4(1.0f, 0.8f, 0.2f, 0.9f); // Default fire color
            float size = 4.0f;
            float lifetime = 0.6f;
            
            // Get texture if one was loaded by config
            wolf::Texture* texture = nullptr;
            if (!m_particles.empty() && m_particles[0].m_texture)
            {
                texture = m_particles[0].m_texture;
            }
            
            // Emit the particle
            Emit(position, velocity, color, size, lifetime, texture);
            
            // Reset timer, accounting for remainder
            m_emissionTimer = fmod(m_emissionTimer, emissionInterval);
        }
    }
    
    // Handle auto-cleanup if enabled
    if (m_autoDestroy)
    {
        // If we currently have active particles, update the flag
        if (anyActive)
        {
            m_cleanupTimer = 0.0f;
        }
        // If all particles are inactive but we previously had active ones
        else
        {
            m_cleanupTimer += delta;
            if (m_cleanupTimer >= m_cleanupGracePeriod)
            {
                GetGameObject()->DeleteComponent<ParticleComponent>();
            }
        }
    }
}

void ParticleComponent::Render()
{
    if (!s_pShader) return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    s_pShader->Bind();

    // Render non-textured particles as points
    RenderPointParticles();
    
    // Render textured particles using quads, grouped by texture for better performance
    RenderTexturedParticles();

    // Cleanup state
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void ParticleComponent::RenderPointParticles()
{
    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    std::vector<float> sizes;
    std::vector<float> rotations;
    
    // Collect active non-textured particles
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && !particle.m_texture)
        {
            positions.push_back(particle.m_pos);
            colors.push_back(particle.m_color);
            sizes.push_back(particle.m_size);
            rotations.push_back(particle.m_rotation);
        }
    }
    
    if (positions.empty())
        return;
        
    glBindVertexArray(m_vao);
    
    // Update VBOs with new data
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_STREAM_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_rotationVBO);
    glBufferData(GL_ARRAY_BUFFER, rotations.size() * sizeof(float), rotations.data(), GL_STREAM_DRAW);
    
    // Set shader uniforms for point particles
    s_pShader->SetUniform("useTexture", 0);
    
    // Draw all point particles in one batch
    glDrawArrays(GL_POINTS, 0, positions.size());
}

void ParticleComponent::RenderTexturedParticles()
{
    // Group particles by texture for efficient rendering
    std::unordered_map<wolf::Texture*, std::vector<const Particle*>> textureGroups;
    
    for (const auto& particle : m_particles)
    {
        if (particle.m_active && particle.m_texture)
        {
            textureGroups[particle.m_texture].push_back(&particle);
        }
    }
    
    if (textureGroups.empty())
        return;
    
    // Bind VAO and set base shader uniforms
    glBindVertexArray(m_quadVAO);
    
    // Render particles grouped by texture
    for (const auto& [texture, particles] : textureGroups)
    {
        if (!texture || texture->GetID() == 0)
            continue;
        
        // Bind texture for this group
        glActiveTexture(GL_TEXTURE0);
        texture->Bind(0);
        
        // Set texture-related uniforms after
        s_pShader->Bind();
        s_pShader->SetUniform("useTexture", 1);
        s_pShader->SetUniform("particleTexture", 0);
        
        // Render each particle
        for (const Particle* particle : particles)
        {
            // set up model matrix for each particle
            float scaleFactor = particle->m_size * 10.0f; // edit param as u wish
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(particle->m_pos, 0.0f));
            model = glm::rotate(model, glm::radians(particle->m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(scaleFactor, scaleFactor, 1.0f));
            
            s_pShader->SetUniform("model", model);
            s_pShader->SetUniform("particleColor", particle->m_color);
            s_pShader->Bind(); // call bind again
            
            // Draw the quad
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
}

void ParticleComponent::Emit(const glm::vec2& position, const glm::vec2& velocity, 
                           const glm::vec4& color, float size, float lifetime, 
                           wolf::Texture* texture, float rotation, float angularVelocity)
{
    // Try to find an inactive particle
    for (auto& particle : m_particles)
    {
        if (!particle.m_active)
        {
            // Reset particle with new values
            particle.Reset(position, velocity, color, size, lifetime, texture);
            
            // Set enhanced properties
            particle.m_rotation = rotation;
            particle.m_angularVelocity = angularVelocity;
            
            // Apply modifiers for new particles
            ApplyModifiers(particle, 0.0f, true);
            
            return; // Found and used an inactive particle, exit
        }
    }
    
    // If we reached here, all particles are active
    wolf::Error("Failed to emit particle - all particles are active. Consider increasing max particles.");
}

void ParticleComponent::EmitBurst(const glm::vec2& position, const glm::vec2& baseVelocity, 
                                const glm::vec4& color, float size, float lifetime, 
                                int count, float spread, wolf::Texture* texture)
{
    // Calculate the spread angle in radians
    float spreadRadians = glm::radians(spread);
    
    // Calculate base angle from velocity
    float baseAngle = atan2(baseVelocity.y, baseVelocity.x);
    
    // Calculate speed from velocity
    float speed = glm::length(baseVelocity);
    
    // Create random engine for variations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(-spreadRadians/2, spreadRadians/2);
    std::uniform_real_distribution<float> sizeDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> lifetimeDist(0.8f, 1.2f);
    
    // Emit particles
    for (int i = 0; i < count; i++)
    {
        // Calculate random direction within spread
        float angle = baseAngle + angleDist(gen);
        glm::vec2 velocity(cos(angle) * speed, sin(angle) * speed);
        
        // Apply some variation to size and lifetime
        float particleSize = size * sizeDist(gen);
        float particleLifetime = lifetime * lifetimeDist(gen);
        
        // Random rotation
        float rotation = std::uniform_real_distribution<float>(0.0f, 360.0f)(gen);
        float angularVelocity = std::uniform_real_distribution<float>(-180.0f, 180.0f)(gen);
        
        // Emit the particle
        Emit(position, velocity, color, particleSize, particleLifetime, 
             texture, rotation, angularVelocity);
    }
}

void ParticleComponent::SetMaxParticles(size_t maxParticles)
{
    m_particles.resize(maxParticles);
}

void ParticleComponent::AddModifier(std::shared_ptr<ParticleModifier> modifier)
{
    if (modifier)
    {
        // Check if a modifier with the same name already exists
        for (size_t i = 0; i < m_modifiers.size(); i++)
        {
            if (m_modifiers[i]->GetName() == modifier->GetName())
            {
                // Replace existing modifier
                m_modifiers[i] = modifier;
                return;
            }
        }
        
        // Add new modifier
        m_modifiers.push_back(modifier);
    }
}

void ParticleComponent::RemoveModifier(const std::string& modifierName)
{
    for (auto it = m_modifiers.begin(); it != m_modifiers.end(); )
    {
        if ((*it)->GetName() == modifierName)
        {
            it = m_modifiers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<ParticleModifier> ParticleComponent::GetModifier(const std::string& modifierName)
{
    for (auto& modifier : m_modifiers)
    {
        if (modifier->GetName() == modifierName)
        {
            return modifier;
        }
    }
    return nullptr;
}

void ParticleComponent::ApplyModifiers(Particle& particle, float delta, bool isNewParticle)
{
    for (auto& modifier : m_modifiers)
    {
        if (modifier->IsEnabled())
        {
            if (isNewParticle)
            {
                modifier->OnEmit(particle);
            }
            else
            {
                modifier->Update(particle, delta);
            }
        }
    }
}

bool ParticleComponent::LoadConfigFromYAML(const std::string& filename)
{
    try {
        // wolf::Log("Attempting to load YAML config from ", filename.c_str());
        
        if (!std::filesystem::exists(filename)) {
            wolf::Error("Config file does not exist: ", filename.c_str());
            return false;
        }
        
        YAML::Node config;
        try {
            config = YAML::LoadFile(filename);
        } catch (const YAML::Exception& e) {
            wolf::Error("YAML parsing error: ", e.what());
            return false;
        }
        
        YAML::Node particleConfig = config["particle_config"];
        
        if (!particleConfig) {
            wolf::Error("Invalid config format in file: ", filename.c_str());
            return false;
        }
        
        // Clear existing modifiers
        m_modifiers.clear();
        
        // Load basic properties if available
        if (particleConfig["max_particles"]) {
            SetMaxParticles(particleConfig["max_particles"].as<size_t>());
        }
        
        // Set default auto-destroy values
        m_autoDestroy = false;
        m_cleanupGracePeriod = 0.5f;
        m_cleanupTimer = 0.0f;
        m_hasActiveParticles = false;

        if (particleConfig["autodestroy"])
            m_autoDestroy = particleConfig["autodestroy"].as<bool>();
        
        // Load modifiers from the YAML file
        if (particleConfig["modifiers"]) {
            YAML::Node modifiers = particleConfig["modifiers"];
            
            // Emission Shape
            if (modifiers["emission_shape"] && modifiers["emission_shape"]["enabled"].as<bool>()) {
                YAML::Node emissionShape = modifiers["emission_shape"];
                auto modifier = std::make_shared<EmissionShapeModifier>();
                
                if (emissionShape["shape_type"])
                    modifier->SetShapeType(static_cast<EmissionShapeModifier::ShapeType>(emissionShape["shape_type"].as<int>()));
                
                if (emissionShape["random_direction"])
                    modifier->SetRandomDirection(emissionShape["random_direction"].as<bool>());
                
                if (emissionShape["direction_angle"])
                    modifier->SetDirectionAngle(emissionShape["direction_angle"].as<float>());
                
                if (emissionShape["spread_angle"])
                    modifier->SetSpreadAngle(emissionShape["spread_angle"].as<float>());
                
                if (emissionShape["emit_from_edge"])
                    modifier->SetEmitFromEdge(emissionShape["emit_from_edge"].as<bool>());
                
                AddModifier(modifier);
            }
            
            // Gravity
            if (modifiers["gravity"] && modifiers["gravity"]["enabled"].as<bool>()) {
                YAML::Node gravity = modifiers["gravity"];
                auto modifier = std::make_shared<GravityModifier>();
                
                if (gravity["gravity_x"] && gravity["gravity_y"])
                    modifier->SetGravity(glm::vec2(gravity["gravity_x"].as<float>(), gravity["gravity_y"].as<float>()));
                
                if (gravity["strength"])
                    modifier->SetStrength(gravity["strength"].as<float>());
                
                AddModifier(modifier);
            }
            
            // Drag
            if (modifiers["drag"] && modifiers["drag"]["enabled"].as<bool>()) {
                YAML::Node drag = modifiers["drag"];
                auto modifier = std::make_shared<DragModifier>();
                
                if (drag["coefficient"])
                    modifier->SetDragCoefficient(drag["coefficient"].as<float>());
                
                AddModifier(modifier);
            }
            
            // Size Over Lifetime
            if (modifiers["size_over_lifetime"] && modifiers["size_over_lifetime"]["enabled"].as<bool>()) {
                YAML::Node sizeOverLifetime = modifiers["size_over_lifetime"];
                auto modifier = std::make_shared<SizeOverLifetimeModifier>();
                
                if (sizeOverLifetime["start_scale"] && sizeOverLifetime["end_scale"])
                    modifier->SetScales(sizeOverLifetime["start_scale"].as<float>(), sizeOverLifetime["end_scale"].as<float>());
                
                if (sizeOverLifetime["curve_type"])
                    modifier->SetCurveType(static_cast<SizeOverLifetimeModifier::CurveType>(sizeOverLifetime["curve_type"].as<int>()));
                
                AddModifier(modifier);
            }
            
            // Color Over Lifetime
            if (modifiers["color_over_lifetime"] && modifiers["color_over_lifetime"]["enabled"].as<bool>()) {
                YAML::Node colorOverLifetime = modifiers["color_over_lifetime"];
                auto modifier = std::make_shared<ColorOverLifetimeModifier>();
                
                if (colorOverLifetime["start_color"] && colorOverLifetime["end_color"]) {
                    auto startColor = colorOverLifetime["start_color"].as<std::vector<float>>();
                    auto endColor = colorOverLifetime["end_color"].as<std::vector<float>>();
                    
                    if (startColor.size() == 4 && endColor.size() == 4) {
                        modifier->SetColors(
                            glm::vec4(startColor[0], startColor[1], startColor[2], startColor[3]),
                            glm::vec4(endColor[0], endColor[1], endColor[2], endColor[3])
                        );
                    }
                }
                
                AddModifier(modifier);
            }
            
            // Rotation
            if (modifiers["rotation"] && modifiers["rotation"]["enabled"].as<bool>()) {
                YAML::Node rotation = modifiers["rotation"];
                auto modifier = std::make_shared<RotationModifier>();
                
                if (rotation["rotation_speed"])
                    modifier->SetRotationSpeed(rotation["rotation_speed"].as<float>());
                
                if (rotation["random_initial"])
                    modifier->SetRandomizeInitialRotation(rotation["random_initial"].as<bool>());
                
                if (rotation["random_direction"])
                    modifier->SetRandomizeRotationDirection(rotation["random_direction"].as<bool>());
                
                AddModifier(modifier);
            }
            
            // Vortex
            if (modifiers["vortex"] && modifiers["vortex"]["enabled"].as<bool>()) {
                YAML::Node vortex = modifiers["vortex"];
                auto modifier = std::make_shared<VortexModifier>();
                
                if (vortex["center_x"] && vortex["center_y"])
                    modifier->SetCenterOffset(glm::vec2(vortex["center_x"].as<float>(), vortex["center_y"].as<float>()));
                
                if (vortex["rotation_speed"])
                    modifier->SetRotationSpeed(vortex["rotation_speed"].as<float>());
                
                if (vortex["strength"])
                    modifier->SetStrength(vortex["strength"].as<float>());
                
                if (vortex["attraction"])
                    modifier->SetAttractionStrength(vortex["attraction"].as<float>());
                
                AddModifier(modifier);
            }
            
            // Attractor
            if (modifiers["attractor"] && modifiers["attractor"]["enabled"].as<bool>()) {
                YAML::Node attractor = modifiers["attractor"];
                auto modifier = std::make_shared<AttractorModifier>();
                
                if (attractor["position_x"] && attractor["position_y"])
                    modifier->SetAttractorOffset(glm::vec2(attractor["position_x"].as<float>(), attractor["position_y"].as<float>()));
                
                if (attractor["strength"])
                    modifier->SetStrength(attractor["strength"].as<float>());
                
                if (attractor["falloff_radius"])
                    modifier->SetFalloffRadius(attractor["falloff_radius"].as<float>());
                
                if (attractor["is_repeller"])
                    modifier->SetIsRepeller(attractor["is_repeller"].as<bool>());
                
                AddModifier(modifier);
            }
        }
        
        // wolf::Log("Particle configuration loaded successfully with ", m_modifiers.size(), " modifiers");
        return true;
    }
    catch (const std::exception& e) {
        wolf::Error("Exception while loading particle config: ", e.what());
        return false;
    }
}