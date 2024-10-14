#pragma once

//-----------------------------------------------------------------------------
// File:			W_Grid2D.h
// Original Author:	D'Anyil Landry
//
// A dense regular 2D grid of arbitrary data.
// 
// Grid values are stored in a flattened 1D array using the following scheme:
// grid(x, y) = flat[y * width + x];
//-----------------------------------------------------------------------------

#include <vector>

namespace wolf
{

template <typename T>
class Grid2D
{

// Interface
public:

    // Create an empty grid from (0, 0) to (width - 1, height - 1)
    Grid2D(int width, int height, const T& initialValue = T())
    {
        Resize(width, height, initialValue);
    }

    // Gets the value at the given grid position
    // NOTE: Doesn't validate coordinates!
    const T& Get(int x, int y) const
    {
        int index = y * m_width + x;
        return m_data[index];
    }

    // Sets the value at the given grid position
    // NOTE: Doesn't validate coordinates!
    void Set(int x, int y, const T& value)
    {
        int index = y * m_width + x;
        m_data[index] = value;
    }

    // Fills the entire grid with the given value
    void Clear(const T& value = T())
    {
        std::fill(m_data.begin(), m_data.end(), value);
    }

    // Resizes the grid to the given dimensions and then clears it with value
    void Resize(int width, int height, const T& value = T())
    {
        m_width = width;
        m_height = height;
        m_data.resize(width * height);
        Clear(value);
    }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    // Returns a const reference to the internal flattened array
    const std::vector<T>& Data()
    {
        return m_data;
    }

// Implementation
private:

    // Flattened array of data
    std::vector<T> m_data;

    // Dimensions
    int m_width;
    int m_height;
};

}