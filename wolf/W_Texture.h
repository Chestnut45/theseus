//-----------------------------------------------------------------------------
// File:			W_Texture.h
// Original Author:	Gordon Wood
//
// Class encapsulating a texture
//-----------------------------------------------------------------------------
#ifndef W_TEXTURE_H
#define W_TEXTURE_H

#include "W_Types.h"
#include <string>

namespace wolf
{
class Texture
{
	friend class TextureManager;

	public:
		//-------------------------------------------------------------------------
		// PUBLIC TYPES
		//-------------------------------------------------------------------------
		enum Format
		{
			FMT_888,
			FMT_8888,
			FMT_565,
			FMT_4444,
			FMT_5551,
			FMT_Depth,
			FMT_RGBA16F,
			FMT_NUM_FORMATS
		};

		enum WrapMode
		{
			WM_Repeat,
			WM_Clamp,
			WM_MirroredRepeat,
			WM_NUM_WRAP_MODES,
			WM_Invalid
		};

		enum FilterMode
		{
			FM_Nearest,
			FM_Linear,
			FM_NearestMipmap,
			FM_LinearMipmap,
			FM_TrilinearMipmap,
			FM_NUM_FILTER_MODES,
			FM_Invalid
		};
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PUBLIC INTERFACE
		//-------------------------------------------------------------------------
		void Bind(int texUnit = 0) const;

		void SetWrapMode(WrapMode uWrap, WrapMode vWrap = WM_Invalid);
		void SetFilterMode(FilterMode minFilter, FilterMode magFilter = FM_Invalid);

		unsigned int GetWidth() const { return m_width; }
		unsigned int GetHeight() const { return m_height; }
		//-------------------------------------------------------------------------

	private:
		//-------------------------------------------------------------------------
		// PRIVATE METHODS
		//-------------------------------------------------------------------------

		// Made private to enforce creation/destruction via TextureManager
		Texture(const std::string& path);
		Texture(void* pData, unsigned int width, unsigned int height, Format fmt);
		Texture(unsigned int width, unsigned int height, Format fmt);
		virtual ~Texture();

		void LoadFromDDS(const std::string& path);
		void LoadImage(const std::string& path);
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PRIVATE MEMBERS
		//-------------------------------------------------------------------------
		GLuint			m_glTex;
		WrapMode		m_uWrap;
		WrapMode		m_vWrap;
		FilterMode		m_minFilter;
		FilterMode		m_magFilter;
		unsigned int	m_width;
		unsigned int    m_height;
		//-------------------------------------------------------------------------
};

}

#endif

