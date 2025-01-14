#pragma once

//-----------------------------------------------------------------------------
// File:			W_Shapes.h
// Original Author:	D'Anyil Landry
//
// Some helper classes for testing intersections with 2D shapes and points.
//-----------------------------------------------------------------------------

#include <glm/glm.hpp>
#include <array>

namespace wolf
{

// 2D Shapes

// Represents a circle with floating point coordinates
struct Circle
{
    // Construct a circle with the given position and radius
    Circle(const glm::vec2& position, float radius);
    ~Circle();

    // Intersection tests
    bool Intersects(const glm::vec2& position) const;
    bool Intersects(const Circle& circle) const;

    // Data
    glm::vec2 m_position;
    float m_radius;
};

// Represents a rectangle with floating point coordinates
struct Rectangle
{
    // Construct a unit square (default)
    Rectangle();

    // Construct a rectangle with the given extents
    Rectangle(float left, float top, float right, float bottom);

    // Construct a rectangle from an origin (top-left) and size (width/height)
    Rectangle(const glm::vec2& topLeft, const glm::vec2& size);

    ~Rectangle();

    // Intersection tests
    bool Intersects(const glm::vec2& position) const;
    bool Intersects(const Rectangle& rectangle) const;

    // Helpers

    // Returns the width of this rectangle
    inline float GetWidth() const { return m_right - m_left; };

    // Returns the height of this rectangle
    inline float GetHeight() const { return m_top - m_bottom; };

    // Returns the top left coordinate of the rectangle
    inline glm::vec2 GetPosition() const { return glm::vec2(m_left, m_top); };

     // **New Helper**: Returns the corners of the rectangle
    inline std::array<glm::vec2, 4> GetCorners() const
    {
        return {
            glm::vec2(m_left, m_top),                // Top-left
            glm::vec2(m_right, m_top),              // Top-right
            glm::vec2(m_left, m_bottom),            // Bottom-left
            glm::vec2(m_right, m_bottom)            // Bottom-right
        };
    }

    // Data
    float m_left;
    float m_top;
    float m_right;
    float m_bottom;
};

// Represents a rectangle with integer coordinates
// NOTE: This uses a different internal representation than
// wolf::Rectangle. Really they should both use the same representation.
// TODO: Update rectangle representation, refactor old code to avoid breakage (if time permits).
struct IRectangle
{
    // Construct a unit square (default)
    IRectangle();

    // Construct a rectangle with the given extents
    IRectangle(int left, int top, int right, int bottom);

    // Construct a rectangle from an origin (bottom-left) and size (width/height)
    IRectangle(const glm::ivec2& bottomLeft, const glm::ivec2& size);

    ~IRectangle();

    // Intersection tests
    bool Intersects(const glm::ivec2& position) const;
    bool Intersects(const IRectangle& rectangle) const;

    // Data

    // The origin of the rectangle (bottom-left)
    glm::ivec2 m_origin;

    // The size of the rectangle (width, height)
    glm::ivec2 m_size;
};

void _ShapeTests();

}