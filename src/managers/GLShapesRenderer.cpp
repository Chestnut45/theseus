//-----------------------------------------------------------------------------
// File: GLShapesRenderer.cpp
// Original Author: Nguyễn Minh Nhật
// Renders lines, triangles & regular polygons.
//-----------------------------------------------------------------------------

#include "GLShapesRenderer.h"

const std::vector<ColouredVertex2D> coloured_vertex = 
{
    {0.0f, 0.0f},
};

GLShapesRenderer* GLShapesRenderer::s_pGLSR = nullptr;

// Initialises singleton
void GLShapesRenderer::CreateInstance()
{
    if (s_pGLSR == nullptr)
    {
        s_pGLSR = new GLShapesRenderer();
    }
    else
    {
        return;
    }
}

// Deletes singleton
void GLShapesRenderer::DestroyInstance()
{
    if(s_pGLSR != nullptr)
    {
        delete s_pGLSR;
        s_pGLSR = nullptr;
    }
    else
    {
        return;
    }
}

GLShapesRenderer* GLShapesRenderer::GetInstance()
{
    return s_pGLSR;
}

void GLShapesRenderer::AddLine(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2)
{
    // Add vertices to vector
    m_vVertices_L.push_back(p_coords_1);
    m_vVertices_L.push_back(p_coords_2);
}

void GLShapesRenderer::AddTriangle(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3)
{
    // Add vertices to vector
    m_vVertices_T.push_back(p_coords_1);
    m_vVertices_T.push_back(p_coords_2);
    m_vVertices_T.push_back(p_coords_3);
}

void GLShapesRenderer::AddQuad(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3, ColouredVertex2D p_coords_4)
{
    this->AddTriangle(p_coords_1, p_coords_2, p_coords_3);
    this->AddTriangle(p_coords_3, p_coords_4, p_coords_1);
}

void GLShapesRenderer::AddQuad(ColouredVertex2D p_lower_left, float p_width, float p_height)
{
    glm::vec4 colour = glm::vec4(p_lower_left.r, p_lower_left.g, p_lower_left.b, p_lower_left.a);
    
    // Create vertices
    ColouredVertex2D upperLeft = {p_lower_left.x, p_lower_left.y + p_height, colour.r, colour.g, colour.b, colour.a};
    ColouredVertex2D lowerRight = {p_lower_left.x + p_width, p_lower_left.y, colour.r, colour.g, colour.b, colour.a};
    ColouredVertex2D upperRight = {p_lower_left.x + p_width, p_lower_left.y + p_height, colour.r, colour.g, colour.b, colour.a};
    
    // Add vertices to vector
    this->AddTriangle(p_lower_left, upperLeft, upperRight);
    this->AddTriangle(upperRight, lowerRight, p_lower_left);
}

// This method does not render the centre, despite taking a centre parameter
// It instead uses the centre position for vertices calculations, and uses the colour data as the colour of the sides
void GLShapesRenderer::AddRegularPolygon(ColouredVertex2D p_centre, float p_radius, int p_sides, float p_angle_offset)
{
    // Constants for degree & rad calculations
    const float FULL_RAD = 2.0f * MATH_PI;
    const float DEGREE_TO_RAD = MATH_PI / 180.0f;
    const float RAD_OFFSET = fmod(abs(p_angle_offset), 360.0f) * DEGREE_TO_RAD;
    const float DEGREE_INTERVAL = 360.0f / p_sides;
    const float RAD_INTERVAL = DEGREE_INTERVAL * DEGREE_TO_RAD;
    const float RAD_INTERVAL_SIN = glm::sin(RAD_INTERVAL);
    const float RAD_INTERVAL_COS = glm::cos(RAD_INTERVAL);

    // Setup data
    glm::vec2 centrePos = glm::vec2(p_centre.x,  p_centre.y);
    glm::vec2 firstVector = glm::vec2(p_radius, 0.0f);
    float currentRadAngle = RAD_OFFSET;
    glm::vec2 currentVertexPos = centrePos + this->GetRotatedVector(firstVector, RAD_OFFSET);
    glm::vec2 lastVertexPos = currentVertexPos;

    // Calculate vertices of polygon
    for(int i = 0; i < p_sides; i ++)
    {
        // Increment angle
        currentRadAngle = fmod(currentRadAngle + RAD_INTERVAL, FULL_RAD);
        
        // Calculate vertices of each side
        lastVertexPos = currentVertexPos;
        currentVertexPos = centrePos + this->GetRotatedVector(firstVector, currentRadAngle);

        // Create vertices
        ColouredVertex2D lastPoint = {lastVertexPos.x, lastVertexPos.y, p_centre.r, p_centre.g, p_centre.b, p_centre.a};
        ColouredVertex2D nextPoint = {currentVertexPos.x, currentVertexPos.y, p_centre.r, p_centre.g, p_centre.b, p_centre.a};
        
        // Add vertices to vector
        m_vVertices_L.push_back(lastPoint);
        m_vVertices_L.push_back(nextPoint);
    }
}

void GLShapesRenderer::RenderAndDeleteLines()
{
    // Return if the lines program is null or there are no line vertices
    if (!m_pProgram_L || m_vVertices_L.size() == 0)
    {
        return;    
    }
    
    // Render lines
    glm::mat4 model = glm::mat4(1.0f);
    m_pProgram_L->SetUniform("model", model);
    m_pProgram_L->Bind();
    m_pDecl_L->Bind();
    m_pVB_L->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColouredVertex2D) * m_vVertices_L.size(), m_vVertices_L.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, m_vVertices_L.size());
    
    // Clear the vector
    m_vVertices_L.clear();
}

void GLShapesRenderer::RenderAndDeleteTriangles()
{
    // Return if the triangles program is null or there are no line vertices
    if (!m_pProgram_T || m_vVertices_T.size() == 0)
    {
        return;    
    }
    
    // Render lines    
    glm::mat4 model = glm::mat4(1.0f);
    m_pProgram_T->SetUniform("model", model);
    m_pProgram_T->Bind();
    m_pDecl_T->Bind();
    m_pVB_T->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColouredVertex2D) * m_vVertices_T.size(), m_vVertices_T.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, m_vVertices_T.size());
    
    // Clear the vector
    m_vVertices_T.clear();
}

GLShapesRenderer::GLShapesRenderer()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    m_pProgram_L = wolf::ProgramManager::CreateProgram("data/shaders/triangles.vsh", "data/shaders/triangles.fsh");
    m_pVB_L = wolf::BufferManager::CreateVertexBuffer(coloured_vertex.data(), sizeof(ColouredVertex2D) * coloured_vertex.size());
    m_pDecl_L = new wolf::VertexDeclaration();
    m_pDecl_L->Begin();
    m_pDecl_L->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
    m_pDecl_L->AppendAttribute(wolf::AT_Color, 4, wolf::CT_Float);
    m_pDecl_L->SetVertexBuffer(m_pVB_L);
    m_pDecl_L->End();

    m_pProgram_T = wolf::ProgramManager::CreateProgram("data/shaders/triangles.vsh", "data/shaders/triangles.fsh");
    m_pVB_T = wolf::BufferManager::CreateVertexBuffer(coloured_vertex.data(), sizeof(ColouredVertex2D) * coloured_vertex.size());
    m_pDecl_T = new wolf::VertexDeclaration();
    m_pDecl_T->Begin();
    m_pDecl_T->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
    m_pDecl_T->AppendAttribute(wolf::AT_Color, 4, wolf::CT_Float);
    m_pDecl_T->SetVertexBuffer(m_pVB_T);
    m_pDecl_T->End();
}

GLShapesRenderer::~GLShapesRenderer()
{
    delete GLShapesRenderer::m_pDecl_L;
    GLShapesRenderer::m_pDecl_L = nullptr;
    wolf::ProgramManager::DestroyProgram(GLShapesRenderer::m_pProgram_L);
    GLShapesRenderer::m_pProgram_L = nullptr;
    wolf::BufferManager::DestroyBuffer(GLShapesRenderer::m_pVB_L);
    GLShapesRenderer::m_pVB_L = nullptr;

    delete GLShapesRenderer::m_pDecl_T;
    GLShapesRenderer::m_pDecl_T = nullptr;
    wolf::ProgramManager::DestroyProgram(GLShapesRenderer::m_pProgram_T);
    GLShapesRenderer::m_pProgram_T = nullptr;
    wolf::BufferManager::DestroyBuffer(GLShapesRenderer::m_pVB_T);
    GLShapesRenderer::m_pVB_T = nullptr;
}

glm::vec2 GLShapesRenderer::GetRotatedVector(glm::vec2 p_vector, float p_rad_angle)
{
    // Calculate sin & cos of rad angle
    const float RAD_ANGLE_SIN = glm::sin(p_rad_angle);
    const float RAD_ANGLE_COS = glm::cos(p_rad_angle);

    // Calculate rotated vector
    glm::vec2 rotatedVector = glm::vec2(0.0f, 0.0f);
    rotatedVector.x = p_vector.x * RAD_ANGLE_COS + p_vector.y * RAD_ANGLE_SIN;
    rotatedVector.y = -p_vector.x * RAD_ANGLE_SIN + p_vector.y * RAD_ANGLE_COS;

    return rotatedVector;
}