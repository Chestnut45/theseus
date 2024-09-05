#pragma once

//-----------------------------------------------------------------------------
// File:			W_RNG.h
// Original Author:	D'Anyil Landry
//
// A class representing a seeded instance of a pseudo random number generator.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <random>

namespace wolf
{

class RNG
{
// Public interface
public:

    RNG(uint32_t seed = 0);
    ~RNG();

    // Default copy constructor/assignment
    RNG(const RNG&) = default;
    RNG& operator=(const RNG&) = default;

    // Default move constructor/assignment
    RNG(RNG&& other) = default;
    RNG& operator=(RNG&& other) = default;

    // Seed management

    // Sets the seed of this RNG instance
    inline void SetSeed(uint32_t seed) { this->m_seed = seed; m_engine.seed(seed); };

    // Gets the seed of this RNG instance
    inline uint32_t GetSeed() const { return m_seed; }

    // Resets to the initial value for the current seed
    inline void Reseed() { m_engine.seed(m_seed); }

    // Basic RNG
    
    // Generates a uniformly distributed boolean
    inline bool FlipCoin() { return (bool)std::uniform_int_distribution<short>{0, 1}(m_engine); };

    // Generates a uniformly distributed float within the range [min, max]
    // NOTE: If max < min, min is always returned as a fail-safe
    float NextFloat(float min, float max);

    // Generates a uniformly distributed int within the range [min, max]
    // NOTE: If max < min, min is always returned as a fail-safe
    int NextInt(int min, int max);

// Data / implementation
private:
    
    uint32_t m_seed;
    std::default_random_engine m_engine;
};

}