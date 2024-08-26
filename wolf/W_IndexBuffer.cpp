//-----------------------------------------------------------------------------
// File:			W_IndexBuffer.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_IndexBuffer.h"

namespace wolf
{
//----------------------------------------------------------
// Constructor
//----------------------------------------------------------
IndexBuffer::IndexBuffer(unsigned int numIndices) : m_lengthInBytes(numIndices*2)
{
	glGenBuffers(1, &m_bufferId);
}

//----------------------------------------------------------
// Destructor
//----------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1,&m_bufferId);
}

//----------------------------------------------------------
// Fills this Index buffer with the given data
//----------------------------------------------------------
void IndexBuffer::Write(const void* pData, int lengthInBytes)
{
	Bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lengthInBytes == -1 ? m_lengthInBytes : lengthInBytes, pData, GL_STATIC_DRAW);
}

//----------------------------------------------------------
// Binds this buffer
//----------------------------------------------------------
void IndexBuffer::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId);
}

}

