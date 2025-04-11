//-----------------------------------------------------------------------------
// File: W_FrameBuffer.h
// Original Author: Nguyễn Minh Nhật
// Framebuffer
//-----------------------------------------------------------------------------

#ifndef W_FRAMEBUFFER_H
#define W_FRAMEBUFFER_H

#include "W_Types.h"
#include "W_Buffer.h"
#include <iostream>

namespace wolf
{
    
class FrameBuffer : public Buffer
{
    friend class BufferManager;

public:
    virtual void Bind();

    void SetTexSize(unsigned int p_iFTexWidth, unsigned int p_iFTexHeight);
    void SetWindowSize(unsigned int p_iWinWidth, unsigned int p_iWinHeight);
    void Blit();

    GLuint GetTextureID() const;

    static void BindDefault();
    
private:
    FrameBuffer(unsigned int p_iTexWidth, unsigned int p_iTexHeight, unsigned int p_iWinWidth, unsigned int p_iWinHeight);
    ~FrameBuffer();

    void CreateTexture();
    void DeleteTexture();
    void CreateDepthBuffer();
    void DeleteDepthBuffer();
    void CheckFrameBuffer();

    virtual void Write(const void *p_pData, int p_iLength = -1);

    GLuint m_uiBuffer;
    GLuint m_uiTex;
    GLuint m_uiDepthBuf;

    unsigned int m_iWinWidth = 1280;
    unsigned int m_iWinHeight = 720;
    int m_iTexWidth = 320;
    int m_iTexHeight = 180;

    bool m_bIsFBOComplete = false;
};

}

#endif