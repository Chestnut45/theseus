#pragma once

#include <glm/glm.hpp>

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

    // Data
    float m_left;
    float m_top;
    float m_right;
    float m_bottom;
};

void _ShapeTests();

}