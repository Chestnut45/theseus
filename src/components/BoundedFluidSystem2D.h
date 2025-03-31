#pragma once

//-----------------------------------------------------------------------------
// File:			BoundedFluidSystem2D.h
// Original Author:	D'Anyil Landry
//
// A renderable game component representing a bounded 2D particle-based
// fluid simulation.
// 
// NOTE: All coordinates are in world space! wolf::Transforms have no effect
// 
// Supports:
// - Colorable fluid, wave velocity, and caustic effects
// - Full transparency
// - Timed spouts (fluid particle emitters)
// - Interpolated radial force application
// - Gravity
// - Static AABB collision (WIP, has some bugs with tunneling, can't be removed)
//-----------------------------------------------------------------------------

// TODO: Look into multithreading - either parallel for through grid
// cells, or give the fluid systems their own thread so they can at
// least run at the same time as the other expensive game logic

// System includes
#include <vector>
#include <unordered_map>

// Library includes
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <GL/glew.h>

// Project includes
#include <W_BaseComponent.h>
#include <W_ProgramManager.h>
#include <W_RNG.h>
#include <W_Shapes.h>

struct FluidParticle
{
    glm::vec2 m_pos;
    glm::vec2 m_vel;
    glm::vec2 m_force;
    float m_density;
    float m_pressure;

    FluidParticle(const glm::vec2& position)
        :
        m_pos(position),
        m_vel(0.0f),
        m_force(0.0f),
        m_density(0.0f),
        m_pressure(0.0f)
    {
    }
};

struct Spout
{
    glm::vec2 m_pos{0.0f};
    float m_lifetime = 0.0f;
    float m_lifespan = 5.0f;
    float m_spawnRate = 1.0f;
    float m_spawnTimer = 0.0f;
};

// Game component representing a bounded 2D particle-based fluid simulation
class BoundedFluidSystem2D : public wolf::BaseComponent
{
public:

    // Creates a fluid system component with the given bounds
    BoundedFluidSystem2D(const wolf::Rectangle& bounds);
    ~BoundedFluidSystem2D();

    // Delete copy constructor/assignment
    BoundedFluidSystem2D(const BoundedFluidSystem2D&) = delete;
    BoundedFluidSystem2D& operator=(const BoundedFluidSystem2D&) = delete;

    // Delete move constructor/assignment
    BoundedFluidSystem2D(BoundedFluidSystem2D&& other) = delete;
    BoundedFluidSystem2D& operator=(BoundedFluidSystem2D&& other) = delete;

    // -------- Simulation --------

    // Update the system by delta seconds
    // TODO: maxSimulationSteps instead of only allowing 1
    void Update(float delta);

    // Apply a radial force at the given position, interpolated linearly by distance
    // NOTE: A negative strength will pull towards the given position
    void ApplyRadialForce(const glm::vec2& position, float radius, float strength);

    // Add static collision zones to the simulation
    // TODO: Support more than AABBs
    void AddStaticCollisionRect(const wolf::Rectangle& rect);

    // Set / Get the gravity state
    void SetGravity(bool value) { m_simulateGravity = value; }
    bool IsGravityEnabled() const { return m_simulateGravity; }

    // -------- Particle management --------

    // Set / Get the particle radius
    void SetParticleRadius(float radius);
    float GetParticleRadius() const { return m_kernelRadius; }

    // Spawns a single particle with the given position and initial velocity
    void SpawnParticle(const glm::vec2& pos, const glm::vec2& vel = glm::vec2(0.0f));

    // Adds a spout that will spawn particles for the given lifespan in seconds
    // TODO: Add options for randomized or set velocities?
    void AddTimedSpout(const glm::vec2& position, float lifespan, int particlesPerSecond);

    // Spawns particles in a dam break configuration
    void SetupDamBreak(int numParticles);

    // Deletes all particles
    void Clear();

    // -------- Rendering / Visuals --------

    // Draw the fluid particles to the currently bound framebuffer
    void Render(float delta);

    // Set / Get the base color of the fluid
    void SetFluidColor(const glm::vec4& color) { m_fluidColor = color; }
    const glm::vec4& GetFluidColor() const { return m_fluidColor; }

    // Set / Get the color of waves / splashes
    void SetWaveColor(const glm::vec4& color) { m_waveColor = color; }
    const glm::vec4& GetWaveColor() const { return m_waveColor; }

    // Set / Get the color of the wave caustics
    void SetCausticColor(const glm::vec4& color) { m_causticColor = color; }
    const glm::vec4& GetCausticColor() const { return m_causticColor; }

    // Set / Get the frequency of the wave caustics [0, 1]
    void SetCausticFrequency(float frequency) { m_causticFrequency = frequency; }
    float GetCausticFrequency() const { return m_causticFrequency; }
    
    // If true, the fluid will be rendered after the lighting
    bool IsIgnoreLighting() const { return m_ignoreLighting; }
    void SetIgnoreLighting(bool value) { m_ignoreLighting = value; }

    // -------- Debug --------

    // Renders a debug editor GUI for interactively controlling the simulation
    void ShowEditor();

private:

    // Simulation data
    wolf::RNG m_rng;
    wolf::Rectangle m_bounds;
    std::vector<FluidParticle> m_particles;
    std::vector<wolf::Rectangle> m_collisionRects;
    std::vector<Spout> m_spouts;

    // Spatial hashing optimization structure
    // NOTE: Maps each grid cell to a list of particle indices contained in the cell
    // TODO: Performance of unordered_map may well be a bottleneck now, measure / profile
    std::unordered_map<glm::ivec2, std::vector<int>> m_spatialMap;

    // Timing
    float m_totalSimTime = 0.0f;
    float m_elapsedTimeSinceLastSimStep = 0.0f;
    float m_targetFrametime = 1.0f / 60;

    // Visual parameters
    // NOTE: Default style is "water"
    glm::vec4 m_fluidColor{0.039f, 0.295f, 0.402f, 0.812f};
    glm::vec4 m_waveColor{0.8f, 0.886f, 0.941f, 0.745f};
    glm::vec4 m_causticColor{0.936f, 0.836f, 0.757f, 0.827f};
    float m_causticFrequency = 0.5f;
    bool m_ignoreLighting = false;

    // NOTE: Precomputed constant formulas for the solver taken from:
    // Schuermann, Lucas V. (Jul 2017). Implementing SPH in 2D. Writing.
    // https://lucasschuermann.com/writing/implementing-sph-in-2d

    // Solver parameters
    float m_restDensity = 280.0f; // Original 300
    float m_gasConstant = 1800.0f; // Original 2000
    float m_kernelRadius = 32.0f; // Original 16
    float m_kernelRadiusSqr = m_kernelRadius * m_kernelRadius;
    float m_particleMass = 4.5f;
    float m_viscosity = 245.0f; // Original 200
    float m_fixedDelta = 0.0005f; // Original 0.0007

    // Smoothing kernels defined in Müller and their gradients
    // Adapted to 2D per "SPH Based Shallow Water Simulation" by Solenthaler et al.
    float m_poly6 = 4.0f / (M_PI * pow(m_kernelRadius, 8.0f));
    float m_spikyGradient = -10.0f / (M_PI * pow(m_kernelRadius, 5.0f));
    float m_viscLaplacian = 40.0f / (M_PI * pow(m_kernelRadius, 5.0f));

    // Simulation parameters
    float m_boundEpsilon = m_kernelRadius / 2;
    float m_boundDamping = -0.5f;
    bool m_simulateGravity = false;
    glm::vec2 m_gravity{0.0f, -9.81f};

    // Returns the closest grid cell for the given position, clamped to valid grid cells
    glm::ivec2 GetGridCell(const glm::vec2& pos);

    // Rendering data / buffer handles
    static inline size_t s_refCount = 0;
    static inline GLuint s_dummyVAO = 0;
    static inline GLuint s_quadVAO = 0;
    static inline GLuint s_quadVBO = 0;
    static inline GLuint s_particleSSBO = 0;
    static inline GLuint s_framebuffer = 0;
    static inline GLuint s_fbColorTex = 0;
    static inline wolf::Program* s_pParticleShader = nullptr;
    static inline wolf::Program* s_pBlendPassShader = nullptr;

    // Resizes the internal framebuffer for fluid rendering
    // NOTE: This is automatically called by Theseus when the window resizes
    static void ResizeFramebuffer(int width, int height);
    friend class Theseus;
};