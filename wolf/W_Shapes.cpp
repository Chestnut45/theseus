#include "W_Shapes.h"

namespace wolf
{

Circle::Circle(const glm::vec2& position, float radius)
    : m_position(position), m_radius(radius)
{
}

Circle::~Circle()
{
}

bool Circle::Intersects(const glm::vec2& position) const
{
    return glm::distance(position, m_position) < m_radius;
}

bool Circle::Intersects(const Circle& circle) const
{
    return glm::distance(m_position, circle.m_position) < (m_radius + circle.m_radius);
}

Rectangle::Rectangle()
    : m_left(0.0f), m_top(1.0f), m_right(1.0f), m_bottom(0.0f)
{
}

Rectangle::Rectangle(float left, float top, float right, float bottom)
    : m_left(left), m_top(top), m_right(right), m_bottom(bottom)
{
}

Rectangle::Rectangle(const glm::vec2& topLeft, const glm::vec2& size)
    : m_left(topLeft.x), m_top(topLeft.y), m_right(topLeft.x + size.x), m_bottom(topLeft.y - size.y)
{
}

Rectangle::~Rectangle()
{
}

bool Rectangle::Intersects(const glm::vec2& position) const
{
    return  position.x >= m_left &&
            position.x <= m_right &&
            position.y <= m_top &&
            position.y >= m_bottom;
}

bool Rectangle::Intersects(const Rectangle& rectangle) const
{
    return  m_left <= rectangle.m_right &&
            m_right >= rectangle.m_left &&
            m_top >= rectangle.m_bottom &&
            m_bottom <= rectangle.m_top;
}

IRectangle::IRectangle()
    : m_origin(0, 0), m_size(1, 1)
{
}

IRectangle::IRectangle(int left, int top, int right, int bottom)
    : m_origin(left, bottom), m_size(right - left, top - bottom)
{
}

IRectangle::IRectangle(const glm::ivec2& bottomLeft, const glm::ivec2& size)
    : m_origin(bottomLeft), m_size(size)
{
}

IRectangle::~IRectangle()
{
}

bool IRectangle::Intersects(const glm::ivec2& position) const
{
    return  position.x >= m_origin.x &&
            position.x <= m_origin.x + m_size.x &&
            position.y <= m_origin.y + m_size.y &&
            position.y >= m_origin.y;
}

bool IRectangle::Intersects(const IRectangle& rectangle) const
{
    return  m_origin.x <= rectangle.m_origin.x + rectangle.m_size.x &&
            m_origin.x + m_size.x >= rectangle.m_origin.x &&
            m_origin.y + m_size.y >= rectangle.m_origin.y &&
            m_origin.y <= rectangle.m_origin.y + rectangle.m_size.y;
}

void _ShapeTests()
{   
    // Unit circle at (0, 0)
    Circle c(glm::vec2(0.0f, 0.0f), 0.5f);

    // Unit circle at (1, 1)
    Circle c1(glm::vec2(1.0f, 1.0f), 0.5f);

    // Unit circle at (0.5, 0.5)
    Circle c2(glm::vec2(0.5f, 0.5f), 0.5f);

    assert(c.Intersects(c2));
    assert(c1.Intersects(c2));
    assert(!(c.Intersects(c1)) && "these should not intersect");
    assert(c.Intersects(glm::vec2(0.25f, 0.25f)));
    assert(!(c.Intersects(glm::vec2(0.5f, 0.5f))) && "these should not intersect");

    // Rectangle from (-1, 2) (top left) to (2, -1) (bottom right)
    Rectangle r(-1, 2, 2, -1);

    // Same rectangle, different definition
    Rectangle r2(glm::vec2(-1, 2) /* top left */, glm::vec2(3, 3) /* width, height */);

    assert(r.Intersects(glm::vec2(0, 0)));
    assert(r2.Intersects(glm::vec2(0, 0)));
    assert(!r.Intersects(glm::vec2(4, 4)));
    assert(!r2.Intersects(glm::vec2(4, 4)));
    assert(r.Intersects(r2));
    assert(r2.Intersects(r));
    assert(!r.Intersects(Rectangle(-3, -3, -2, -2)));

    // Integer rectangle
    IRectangle r3(-1, 2, 2, -1);

    // Same rectangle, different definition
    IRectangle r4(glm::ivec2(-1, -1) /* bottom left */, glm::ivec2(3, 3) /* width, height */);

    assert(r3.Intersects(glm::vec2(0, 0)));
    assert(r4.Intersects(glm::vec2(0, 0)));
    assert(!r3.Intersects(glm::vec2(4, 4)));
    assert(!r4.Intersects(glm::vec2(4, 4)));
    assert(r3.Intersects(r4));
    assert(r4.Intersects(r3));
    assert(!r3.Intersects(IRectangle(-3, -3, -2, -2)));

    // Ensure glancing intersections work
    IRectangle r5(0, 1, 1, 0);
    IRectangle r6(1, 2, 2, 1);
    assert(r5.Intersects(r6));
    assert(r5.Intersects(glm::ivec2(0, 0)));
    assert(r5.Intersects(glm::ivec2(1, 1)));
}

}