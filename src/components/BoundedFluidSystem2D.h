#pragma once

//-----------------------------------------------------------------------------
// File:			BoundedFluidSystem2D.h
// Original Author:	D'Anyil Landry
//
// A game component representing a bounded 2D particle-based fluid simulation
// 
// TODO: Spatial hashing based on kernel radius for optimization if needed
//-----------------------------------------------------------------------------

#include <vector>

#include <glm/vec2.hpp>

#include <W_BaseComponent.h>
#include <W_ProgramManager.h>
#include <W_RNG.h>
#include <W_Shapes.h>

#include <GL/glew.h>

// Particle structure used in the fluid system
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

    // Integrate the particles forward in time by delta seconds
    // NOTE: If delta > FIXED_DELTA * MAX_INTEGRATION_STEPS_PER_UPDATE,
    // the simulation will slow down!
    void Update(float delta);

    // Draw the fluid particles to the current framebuffer
    void Render(float delta);

    // Apply a radial force at the given position, interpolated linearly by distance
    // NOTE: A negative strength will pull towards the given position
    void ApplyRadialForce(const glm::vec2& position, float radius, float strength);

private:

    // Data members
    wolf::RNG m_rng;
    wolf::Rectangle m_bounds;
    std::vector<FluidParticle> m_particles;

    // Reference counter for static resources
    static inline size_t s_refCount = 0;

    // Rendering data / buffer handles
    static inline GLuint s_quadVAO = 0;
    static inline GLuint s_quadVBO = 0;
    static inline GLuint s_particleSSBO = 0;
    static inline wolf::Program* s_pShader = nullptr;

    // Numerical integration step, using Euler's method
    // NOTE: This is a fixed time-step method, and may be called several
    // times per-frame if delta is large. A maximum number of steps can
    // be chosen by changing MAX_INTEGRATION_STEPS_PER_UPDATE and recompiling
    void Integrate();

    // Constants
    static const int MAX_INTEGRATION_STEPS_PER_UPDATE = 20;

    // NOTE: Precomputed constants for the solver taken from:
    // Schuermann, Lucas V. (Jul 2017). Implementing SPH in 2D. Writing.
    // https://lucasschuermann.com/writing/implementing-sph-in-2d

    // TODO: Parameterize viscosity / mass / radius so users can tweak the system's behaviour

    // TODO: GUI for tweaking these parameters live, testing initial configs, etc.

    // Solver parameters
    static const inline glm::vec2 GRAVITY{0.0f, -9.81f};
    static const inline float REST_DENSITY = 230.0f; // Original 300
    static const inline float GAS_CONSTANT = 1700.0f; // Original 2000
    static const inline float KERNEL_RADIUS = 32.0f; // Original 16
    static const inline float KERNEL_RADIUS_SQR = KERNEL_RADIUS * KERNEL_RADIUS;
    static const inline float PARTICLE_MASS = 2.5f;
    static const inline float VISCOSITY = 200.0f; // Original 200
    static const inline float FIXED_DELTA = 0.0007f;

    // Smoothing kernels defined in Müller and their gradients
    // Adapted to 2D per "SPH Based Shallow Water Simulation" by Solenthaler et al.
    static const inline float POLY6 = 4.0f / (M_PI * pow(KERNEL_RADIUS, 8.0f));
    static const inline float SPIKY_GRAD = -10.0f / (M_PI * pow(KERNEL_RADIUS, 5.0f));
    static const inline float VISC_LAP = 40.0f / (M_PI * pow(KERNEL_RADIUS, 5.0f));

    // Simulation parameters
    static const inline float BOUND_EPSILON = KERNEL_RADIUS;
    static const inline float BOUND_DAMPING = -0.5f;
};