//-----------------------------------------------------------------------------
// File:			W_TextureManager.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_TextureManager.h"

namespace wolf
{

std::map<std::string, TextureManager::Entry*>	TextureManager::m_textures;

//----------------------------------------------------------
// Creates a new texture or returns an existing copy if already
// loaded previously
//----------------------------------------------------------
Texture* TextureManager::CreateTexture(const std::string& path)
{
	std::map<std::string, Entry*>::iterator iter = m_textures.find(path);

	if( iter != m_textures.end() )
	{
		iter->second->m_refCount++;
		return iter->second->m_pTex;
	}

	Texture* pTex = new Texture(path);
	Entry* pEntry = new Entry(pTex);
	m_textures[path] = pEntry;
	return pTex;
}

//----------------------------------------------------------
// When creating directly from data, we don't check for duplicates
// and just delegate directly to the texture class
//----------------------------------------------------------
Texture* TextureManager::CreateTexture(void* pData, unsigned int width, unsigned int height, Texture::Format fmt)
{
	return new Texture(pData, width, height, fmt);
}

//----------------------------------------------------------
// Destroys a texture. Only actually deletes it if the refcount
// is down to 0.
//----------------------------------------------------------
void TextureManager::DestroyTexture(Texture* pTex)
{
	std::map<std::string, Entry*>::iterator iter;
	for( iter = m_textures.begin(); iter != m_textures.end(); iter++ )
	{
		if( iter->second->m_pTex == pTex )
		{
			iter->second->m_refCount--;
			if( iter->second->m_refCount == 0 )
			{
				delete iter->second->m_pTex;
				m_textures.erase(iter);
			}
			return;
		}
	}

	// If we got here, it can only be via a from-data texture which wasn't in
	// the list (unless the pointer being passed in is dodgy in which case we're
	// in trouble anyway).
	delete pTex;
}

}


