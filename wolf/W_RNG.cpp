//-----------------------------------------------------------------------------
// File:			W_RNG.cpp
// Original Author:	D'Anyil Landry
//
// A class representing a seeded instance of a pseudo random number generator.
//-----------------------------------------------------------------------------

#include "W_RNG.h"

namespace wolf
{

RNG::RNG(uint32_t seed)
    : m_seed(seed), m_engine(seed)
{
}

RNG::~RNG()
{
}

float RNG::NextFloat(float min, float max)
{
    if (max < min) return min;
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_engine);
}

int RNG::NextInt(int min, int max)
{
    if (max < min) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_engine);
}

}