//-----------------------------------------------------------------------------
// File:			W_Common.h
// Original Author:	Gordon Wood
//
// Common functions used in all samples
//-----------------------------------------------------------------------------
#ifndef COMMON_H
#define COMMON_H

#include "W_Types.h"
#include <string>

#ifdef DEBUG
#define GL_CHECK_ERROR() { GLenum e = glGetError(); if( e != GL_NO_ERROR ) { printf("GL ERROR: %x\n", e); } }
#else
#define GL_CHECK_ERROR() 
#endif


namespace wolf
{
// Loads in a whole file and returns the contents. User is responsible for then
// delete[]-ing the data. Returns 0 if file not able to be loaded.
void* LoadWholeFile(const std::string& filename, int* pOutFileSize = nullptr);

// Loads in a vertex and pixel shader and combines them into a program
GLuint LoadShaders(const std::string& vsFile, const std::string& fsFile);

bool LoadTGA(const std::string& filename, unsigned int* pWidth, unsigned int* pHeight, unsigned char** ppData);
GLuint CreateTextureFromDDS(const std::string& filename, unsigned int* pWidth = 0, unsigned int* pHeight = 0, bool* pHasMips = 0);
}

#endif
