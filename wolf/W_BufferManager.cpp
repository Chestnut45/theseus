//-----------------------------------------------------------------------------
// File:			W_BufferManager.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_BufferManager.h"

namespace wolf
{

//----------------------------------------------------------
// Creates a new Vertex Buffer
//----------------------------------------------------------
VertexBuffer* BufferManager::CreateVertexBuffer(unsigned int length)
{
	return new VertexBuffer(length);
}

//----------------------------------------------------------
// Creates a new Vertex Buffer
//----------------------------------------------------------
VertexBuffer* BufferManager::CreateVertexBuffer(const void* pData, unsigned int length)
{
	return new VertexBuffer(pData,length);
}

//----------------------------------------------------------
// Creates a new Index Buffer
//----------------------------------------------------------
IndexBuffer* BufferManager::CreateIndexBuffer(unsigned int numIndices)
{
	return new IndexBuffer(numIndices);
}

//----------------------------------------------------------
// Creates a new Index Buffer
//----------------------------------------------------------
IndexBuffer* BufferManager::CreateIndexBuffer(const unsigned short* pData, unsigned int numIndices)
{
	IndexBuffer* pRet = new IndexBuffer(numIndices);
	pRet->Write(pData);
	return pRet;
}

FrameBuffer *BufferManager::CreateFrameBuffer(unsigned int p_iFBOTexWidth, unsigned int p_iFBOTexHeight, unsigned int p_iWinWidth, unsigned int p_iWinHeight)
{
	return new FrameBuffer(p_iFBOTexWidth, p_iFBOTexHeight, p_iWinWidth, p_iWinHeight);
}

//----------------------------------------------------------
// Destroys a buffer. 
//----------------------------------------------------------
void BufferManager::DestroyBuffer(Buffer* pBuf)
{
	if(!pBuf)
		return;
	delete pBuf;
}

}




