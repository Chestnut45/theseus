//-----------------------------------------------------------------------------
// File: GLShapesRenderer. pp
// Original Author: Nguyễn Minh Nhật
// Renders Points, Lines & Triangles.
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
    m_vVertices_L.push_back(p_coords_1);
    m_vVertices_L.push_back(p_coords_2);
}

void GLShapesRenderer::AddTriangle(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3)
{
    m_vVertices_T.push_back(p_coords_1);
    m_vVertices_T.push_back(p_coords_2);
    m_vVertices_T.push_back(p_coords_3);
}

void GLShapesRenderer::AddQuad(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3, ColouredVertex2D p_coords_4)
{
    this->AddTriangle(p_coords_1, p_coords_2, p_coords_3);
    this->AddTriangle(p_coords_3, p_coords_4, p_coords_1);
}

void GLShapesRenderer::RenderAndDeleteLines()
{
    if (!m_pProgram_L || m_vVertices_L.size() == 0)
    {
        return;    
    }
    //printf("GLSR - RenDel_L\n");
        
    glm::mat4 model = glm::mat4(1.0f);
    m_pProgram_L->SetUniform("model", model);
    m_pProgram_L->Bind();
    m_pDecl_L->Bind();
    m_pVB_L->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColouredVertex2D) * m_vVertices_L.size(), m_vVertices_L.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, m_vVertices_L.size());
    m_vVertices_L.clear();
}

void GLShapesRenderer::RenderAndDeleteTriangles()
{
    if (!m_pProgram_T || m_vVertices_T.size() == 0)
    {
        return;    
    }
    //printf("GLSR - RenDel_T\n");
    //std::cout << "GLSR - Vertex Count: " << m_vVertices_T.size() << std::endl;
        
    glm::mat4 model = glm::mat4(1.0f);
    m_pProgram_T->SetUniform("model", model);
    m_pProgram_T->Bind();
    m_pDecl_T->Bind();
    m_pVB_T->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColouredVertex2D) * m_vVertices_T.size(), m_vVertices_T.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, m_vVertices_T.size());
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