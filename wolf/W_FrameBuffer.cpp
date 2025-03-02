//-----------------------------------------------------------------------------
// File: W_FrameBuffer.cpp
// Original Author: Nguyễn Minh Nhật
// Framebuffer
//-----------------------------------------------------------------------------

#include "W_FrameBuffer.h"

namespace wolf
{

//------------------//
//  PUBLIC METHODS  //
//------------------//

void FrameBuffer::Bind()
{
    if(m_bIsFBOComplete == false) return;
    
    glViewport(0, 0, m_iTexWidth, m_iTexHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_uiBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FrameBuffer::Write(const void *p_pData, int p_iLength)
{
}

void FrameBuffer::SetTexSize(unsigned int p_iTexWidth, unsigned int p_iTexHeight)
{
    if(p_iTexWidth == 0 || p_iTexHeight == 0) return;
    if(this->m_iTexWidth == p_iTexWidth && this->m_iTexHeight == p_iTexHeight) return;

    this->m_iTexWidth = p_iTexWidth;
    this->m_iTexHeight = p_iTexHeight;

    glBindFramebuffer(GL_FRAMEBUFFER, m_uiBuffer);

    DeleteTexture();
    CreateTexture();

    DeleteDepthBuffer();
    CreateDepthBuffer();   
    CheckFrameBuffer();
    BindDefault();
}

void FrameBuffer::SetWindowSize(unsigned int p_iWinWidth, unsigned int p_iWinHeight)
{
    if(p_iWinWidth == 0 || p_iWinHeight == 0) return;
    if(this->m_iWinWidth == p_iWinWidth && this->m_iTexHeight == p_iWinHeight) return;

    this->m_iWinWidth = p_iWinWidth;
    this->m_iWinHeight = p_iWinHeight;
}

void FrameBuffer::Blit()
{
    if(m_bIsFBOComplete == false) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_uiBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_iTexWidth, m_iTexHeight,
                        0, 0, m_iWinWidth, m_iWinHeight,
                        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                        GL_NEAREST);
    glViewport(0, 0, m_iWinWidth, m_iWinHeight);
}

GLuint FrameBuffer::GetTextureID() const
{
    return m_uiTex;
}


void FrameBuffer::BindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//-------------------//
//  PRIVATE METHODS  //
//-------------------//

FrameBuffer::FrameBuffer(unsigned int p_iTexWidth, unsigned int p_iTexHeight, unsigned int p_iWinWidth, unsigned int p_iWinHeight)
{
    m_iTexWidth = p_iTexWidth != 0 ? p_iTexWidth : 320;
    m_iTexHeight = p_iTexHeight  != 0 ? p_iTexHeight : 200;

    m_iWinWidth = p_iWinWidth;
    m_iWinHeight = p_iWinHeight;

    glGenFramebuffers(1, &m_uiBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_uiBuffer);
    
    CreateTexture();
    CreateDepthBuffer();
    CheckFrameBuffer();
    BindDefault();
}

FrameBuffer::~FrameBuffer()
{
    glDeleteFramebuffers(1, &m_uiBuffer);
}

void FrameBuffer::CreateTexture()
{
    if(m_uiTex != 0)
    {
        DeleteTexture();
    }
    glGenTextures(1, &m_uiTex);
    glBindTexture(GL_TEXTURE_2D, m_uiTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_iTexWidth, m_iTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiTex, 0);
}

void FrameBuffer::DeleteTexture()
{
    if (m_uiTex != 0) {
        glDeleteTextures(1, &m_uiTex);
        m_uiTex = 0;
    }
}

void FrameBuffer::CreateDepthBuffer()
{
    if (m_uiDepthBuf != 0)
    {
        DeleteDepthBuffer();
    }
    glGenRenderbuffers(1, &m_uiDepthBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, m_uiDepthBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_iTexWidth, m_iTexHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_uiDepthBuf);
}

void FrameBuffer::DeleteDepthBuffer()
{
    if (m_uiDepthBuf != 0) {
        glDeleteRenderbuffers(1, &m_uiDepthBuf);
        m_uiDepthBuf = 0;
    }
}

void FrameBuffer::CheckFrameBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_uiBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "INCOMPLETE FBO" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_bIsFBOComplete = false;
        return;
    }

    m_bIsFBOComplete = true;
}

}