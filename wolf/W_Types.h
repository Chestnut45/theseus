//-----------------------------------------------------------------------------
// File:			W_Types.h
// Original Author:	Gordon Wood
//
// Common types throughout wolf (and some defines too)
//-----------------------------------------------------------------------------
#ifndef W_TYPES_H
#define W_TYPES_H

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Dear ImGui: https://github.com/ocornut/imgui
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

// Font awesome icons: https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-regular-400.ttf
#include <IconsFontAwesome6.h>

namespace wolf
{
#ifndef MATH_PI
#define MATH_PI 3.141592654f
#endif

inline float min(float a, float b)
{
	return a < b ? a : b;
}

inline float max(float a, float b)
{
	return a > b ? a : b;
}

inline float randFloat()
{
	return (float)rand() / (float)RAND_MAX;
}

inline float randFloat(float min, float max)
{
	return min + (max-min) * randFloat();
}

// Our attribute indices 
enum Attribute
{
	AT_Position = 0,
	AT_Color,
	AT_TexCoord1,
	AT_TexCoord2,
	AT_TexCoord3,
	AT_TexCoord4,
	AT_TexCoord5,
	AT_TexCoord6,
	AT_TexCoord7,
	AT_TexCoord8,
	AT_Normal,
	AT_Tangent,
	AT_BiTangent,
	AT_BoneIndices,
	AT_BoneWeights,
	AT_NUM_ATTRIBS
};

enum ComponentType
{
	CT_Float = 0,
	CT_Int,	
	CT_UInt,	
	CT_Byte,	
	CT_UByte,
	CT_UByte4,
	CT_ByteNorm,
	CT_UByteNorm,
	CT_Short,	
	CT_UShort,
	CT_ShortNorm,
	CT_UShortNorm,
	AT_NUM_COMPONENT_TYPES,
	CT_Invalid
};

enum DepthFunc
{
	DF_Never,
	DF_Less,
	DF_LessEqual,
	DF_Equal,
	DF_Greater,
	DF_GreaterEqual,
	DF_NotEqual,
	DF_Always,
	DF_NUM_DEPTH_FUNCS
};

enum BlendMode
{
	BM_SrcAlpha,			
	BM_One,				
	BM_SrcColor,	
	BM_OneMinusSrcColor,
	BM_OneMinusSrcAlpha,
	BM_DstAlpha,	
	BM_OneMinusDstAlpha,
	BM_DstColor,	
	BM_OneMinusDstColor,
	BM_Zero,		
	BM_NUM_BLEND_MODES
};

enum BlendEquation
{
	BE_Add,
	BE_Subtract,
	BE_ReverseSubtract,
	BE_NUM_BLEND_EQUATIONS
};

struct Color4
{
	float r,g,b,a;

	Color4(float p_fR, float p_fG, float p_fB, float p_fA) : r(p_fR), g(p_fG), b(p_fB), a(p_fA) {}
};

#ifndef PI
#define PI		3.141592653589793238f
#endif
    
#define DEG_TO_RAD(d)  (((d)/180.0f)*PI)
    
}

#endif

