//-----------------------------------------------------------------------------
// File: GLShapesRenderer.h
// Original Author: Nguyễn Minh Nhật
// Renders Points, Lines & Triangles.
//-----------------------------------------------------------------------------

#pragma once
#include <wolf.h>
#include "VertexDeclarations.h"
class GLShapesRenderer
{
public:
    static void CreateInstance();
    static void DestroyInstance();

    static GLShapesRenderer* GetInstance();

    void AddLine(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2);
    void AddTriangle(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3);
    void AddQuad(ColouredVertex2D p_coords_1, ColouredVertex2D p_coords_2, ColouredVertex2D p_coords_3, ColouredVertex2D p_coords_4);
    
    void RenderAndDeleteLines();
    void RenderAndDeleteTriangles();
private:
    // Line-rendering tools
    std::vector<ColouredVertex2D> m_vVertices_L;
    wolf::VertexDeclaration *m_pDecl_L = nullptr;
    wolf::Program *m_pProgram_L = nullptr;
    wolf::VertexBuffer *m_pVB_L = nullptr;

    // Triangle-rendering tools
    std::vector<ColouredVertex2D> m_vVertices_T;
    wolf::VertexDeclaration *m_pDecl_T = nullptr;
    wolf::Program *m_pProgram_T = nullptr;
    wolf::VertexBuffer *m_pVB_T = nullptr;

    static GLShapesRenderer* s_pGLSR;

    GLShapesRenderer();
    ~GLShapesRenderer();
};