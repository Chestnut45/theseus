//-----------------------------------------------------------------------------
// File:			W_Texture.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_Texture.h"
#include "W_Common.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace wolf
{

static GLenum gs_aInternalFormatMap[Texture::FMT_NUM_FORMATS] =
{
	GL_RGB,					//FMT_888,
	GL_RGBA,				//FMT_8888,
	GL_RGB,					//FMT_565,
	GL_RGBA,				//FMT_4444,
	GL_RGBA,				//FMT_5551
	GL_DEPTH_COMPONENT24, 	//FMT_Depth
	GL_RGBA16F				//FMT_RGBA16F
};

static GLenum gs_aFormatMap[Texture::FMT_NUM_FORMATS] =
{
	GL_RGB,				//FMT_888,
	GL_RGBA,			//FMT_8888,
	GL_RGB,				//FMT_565,
	GL_RGBA,			//FMT_4444,
	GL_RGBA,			//FMT_5551
	GL_DEPTH_COMPONENT, //FMT_Depth
	GL_RGBA, 			//FMT_RGBA16F
};

static GLenum gs_aTypeMap[Texture::FMT_NUM_FORMATS] =
{
	GL_UNSIGNED_BYTE,			//FMT_888,
	GL_UNSIGNED_BYTE,			//FMT_8888,
	GL_UNSIGNED_SHORT_5_6_5,	//FMT_565,
	GL_UNSIGNED_SHORT_4_4_4_4,	//FMT_4444,
	GL_UNSIGNED_SHORT_5_5_5_1,	//FMT_5551
	GL_UNSIGNED_INT,			//FMT_Depth
	GL_FLOAT					//FMT_RGBA16F
};

static GLenum gs_aWrapMap[Texture::WM_NUM_WRAP_MODES] =
{
	GL_REPEAT,					//WM_Repeat,
	GL_CLAMP_TO_EDGE,			//WM_Clamp,
	GL_MIRRORED_REPEAT			//WM_MirroredRepeat,
};

//----------------------------------------------------------
// Constructor, taking in filename to load
//----------------------------------------------------------
Texture::Texture(const std::string& path) 
	: m_minFilter(Texture::FM_Invalid), m_magFilter(Texture::FM_Invalid),
	  m_uWrap(Texture::WM_Invalid), m_vWrap(Texture::WM_Invalid),
	  m_width(0), m_height(0), m_glTex(0)
{
	if( path.find(".dds") != std::string::npos )
		LoadFromDDS(path);
	else if( path.find(".tga") != std::string::npos || path.find(".png") != std::string::npos)
		LoadImage(path);
	else
	{
		printf("ERROR: No idea how to load this file - %s!", path.c_str());
	}

	SetWrapMode(WM_Clamp);
}

//----------------------------------------------------------
// Constructor taking in data already in ram
//----------------------------------------------------------
Texture::Texture(void* pData, unsigned int width, unsigned int height, Format fmt)
	: m_minFilter(Texture::FM_Invalid), m_magFilter(Texture::FM_Invalid),
	  m_uWrap(Texture::WM_Invalid), m_vWrap(Texture::WM_Invalid),
	  m_width(0), m_height(0), m_glTex(0)
{
	glGenTextures(1,&m_glTex);
	glBindTexture(GL_TEXTURE_2D, m_glTex);

	glTexImage2D(GL_TEXTURE_2D, 0, gs_aInternalFormatMap[fmt], width, height, 0, gs_aFormatMap[fmt], gs_aTypeMap[fmt], pData);

	m_width = width;
	m_height = height;

	SetWrapMode(WM_Clamp);

	glGenerateMipmap(GL_TEXTURE_2D);
	SetFilterMode(FM_TrilinearMipmap, FM_Linear);
}

Texture::Texture(unsigned int width, unsigned int height, Format fmt)
	: m_minFilter(Texture::FM_Invalid), m_magFilter(Texture::FM_Invalid),
	  m_uWrap(Texture::WM_Invalid), m_vWrap(Texture::WM_Invalid),
	  m_width(0), m_height(0), m_glTex(0)
{
	glGenTextures(1,&m_glTex);
	glBindTexture(GL_TEXTURE_2D, m_glTex);

	glTexImage2D(GL_TEXTURE_2D, 0, gs_aInternalFormatMap[fmt], width, height, 0, gs_aFormatMap[fmt], gs_aTypeMap[fmt], nullptr);

	m_width = width;
	m_height = height;
}

//----------------------------------------------------------
// Destructor
//----------------------------------------------------------
Texture::~Texture()
{
	glDeleteTextures(1,&m_glTex);
}

//----------------------------------------------------------
// Binds this texture as the current one (on the currently
// active texture unit)
//----------------------------------------------------------
void Texture::Bind(int texUnit) const
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, m_glTex);
}

//----------------------------------------------------------
// Builds the texture from the given DDS file, including mipmap
// levels, if found in the DDS
//----------------------------------------------------------
void Texture::LoadFromDDS(const std::string& path)
{
	bool bHasMips = false;
	m_glTex = wolf::CreateTextureFromDDS(path, &m_width, &m_height, &bHasMips);

	if( bHasMips )
		m_minFilter = FM_TrilinearMipmap;
	else
		m_minFilter = FM_Linear;

	m_magFilter = FM_Linear;
}

//----------------------------------------------------------
// Builds the texture from the given TGA file. Mipmap levels
// are automatically generated
//----------------------------------------------------------
void Texture::LoadImage(const std::string& path)
{
	glGenTextures(1,&m_glTex);
	glBindTexture(GL_TEXTURE_2D, m_glTex);

    int w,h,n;
    unsigned char* pData = stbi_load(path.c_str(), &w, &h, &n, 0);
	int format = -1;
	int internalFormat = -1;
	switch(n) 
	{
		case 1:
			format = GL_RED;
			internalFormat = GL_R8;
			break;
		case 2:
			format = GL_RG;
			internalFormat = GL_RG;
			break;
		case 3:
			format = GL_RGB;
			internalFormat = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			internalFormat = GL_RGBA;
			break;
		default:
			printf("Error: unknown image format %d\n", n);
			return;
	}

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, pData);

    m_width = w;
	m_height = h;

    stbi_image_free(pData);

	glGenerateMipmap(GL_TEXTURE_2D);
	SetFilterMode(FM_TrilinearMipmap, FM_Linear);
}

//----------------------------------------------------------
// Sets the wrapping mode on U and V to use for this texture when
// it is bound
//----------------------------------------------------------
void Texture::SetWrapMode(WrapMode uWrap, WrapMode vWrap)
{
	if( vWrap == WM_Invalid )
		vWrap = uWrap;

	if( m_uWrap == uWrap && m_vWrap == vWrap )
		return; // Nothing to do, move along!

    Bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gs_aWrapMap[uWrap]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gs_aWrapMap[vWrap]);
	m_uWrap = uWrap;
	m_vWrap = vWrap;
}

//----------------------------------------------------------
// Sets the filtermode (min and mag) to use for this texture when
// it is bound
//----------------------------------------------------------
void Texture::SetFilterMode(FilterMode minFilter, FilterMode magFilter)
{
    if( magFilter == FM_Invalid )
		magFilter = minFilter;

	if( m_minFilter == minFilter && m_magFilter == magFilter )
		return;

	switch( minFilter )
	{
		case FM_Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			break;
		case FM_Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			break;
		case FM_NearestMipmap:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			break;
		case FM_TrilinearMipmap:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			break;
		case FM_LinearMipmap:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			break;
        default: ;
	}

	switch( magFilter )
	{
		case FM_Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case FM_Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
        default: ;            
	}

	m_minFilter = minFilter;
	m_magFilter = magFilter;
}

}

