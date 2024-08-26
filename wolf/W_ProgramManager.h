//-----------------------------------------------------------------------------
// File:			W_ProgramManager.h
// Original Author:	Gordon Wood
//
// Class to manage Programs. Handles creation of them and dealing with duplicate
// requests for the same Program. Also provides a place for a central means 
// to reload all Programs when a context is lost (in a potential future update)
//-----------------------------------------------------------------------------
#ifndef W_PROGRAM_MANAGER_H
#define W_PROGRAM_MANAGER_H

#include "W_Types.h"
#include "W_Program.h"
#include <string>
#include <map>

namespace wolf
{
class ProgramManager
{
	public:
		//-------------------------------------------------------------------------
		// PUBLIC INTERFACE
		//-------------------------------------------------------------------------
		static Program* CreateProgram(const std::string& vsFile, const std::string& fsFile);
		static void DestroyProgram(Program* pTex);

		// TODO: You should really have a method like "Cleanup" that will delete
		// any leftover programs that weren't destroyed by the game, as
		// a safeguard - or at least prints a warning.
		//-------------------------------------------------------------------------

	private:
		//-------------------------------------------------------------------------
		// PRIVATE TYPES
		//-------------------------------------------------------------------------
		struct Entry
		{
			Program*	m_pProg;
			int			m_iRefCount;
			Entry(Program* pProg) : m_pProg(pProg), m_iRefCount(1) {}
		};
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PRIVATE MEMBERS
		//-------------------------------------------------------------------------
		static std::map< std::pair<std::string,std::string>, Entry*>	m_programs;
		//-------------------------------------------------------------------------
};

}

#endif



