//-----------------------------------------------------------------------------
// File:			W_MaterialManager.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_MaterialManager.h"

namespace wolf
{

std::map<std::string, MaterialManager::Entry*>	MaterialManager::m_materials;

//----------------------------------------------------------
// Creates a new material or returns an existing copy if already
// created previously
//----------------------------------------------------------
Material* MaterialManager::CreateMaterial(const std::string& name)
{
	std::map<std::string, Entry*>::iterator iter = m_materials.find(name);

	if( iter != m_materials.end() )
	{
		iter->second->m_iRefCount++;
		return iter->second->m_pMat;
	}

	Material* pMat = new Material(name);
	Entry* pEntry = new Entry(pMat);
	m_materials[name] = pEntry;
	return pMat;
}

//----------------------------------------------------------
// Destroys a material. Only actually deletes it if the refcount
// is down to 0.
//----------------------------------------------------------
void MaterialManager::DestroyMaterial(Material* pMat)
{
	if(!pMat)
		return;

	std::map<std::string, Entry*>::iterator iter;
	for( iter = m_materials.begin(); iter != m_materials.end(); iter++ )
	{
		if( iter->second->m_pMat == pMat )
		{
			iter->second->m_iRefCount--;
			if( iter->second->m_iRefCount == 0 )
			{
				delete iter->second->m_pMat;
				m_materials.erase(iter);
			}
			return;
		}
	}

	// Should never get here
	assert(false);
}

}



