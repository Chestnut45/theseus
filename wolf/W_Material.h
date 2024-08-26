//-----------------------------------------------------------------------------
// File:			W_Material.h
// Original Author:	Gordon Wood
//
// Class representing a material, which is a combination of state + shaders
// + uniforms
//-----------------------------------------------------------------------------
#ifndef W_MATERIAL_H
#define W_MATERIAL_H

#include "W_Types.h"
#include "W_TextureManager.h"
#include "W_ProgramManager.h"
#include <string>
#include <map>
#include <set>

namespace wolf
{
class Material
{
	friend class MaterialManager;
	public:
		//-------------------------------------------------------------------------
		// PUBLIC INTERFACE
		//-------------------------------------------------------------------------
		void Apply();

		void SetUniform(const std::string& name, const glm::mat4& m);
		void SetUniform(const std::string& name, const glm::mat3& m);
		void SetUniform(const std::string& name, const glm::vec4& m);
		void SetUniform(const std::string& name, const glm::vec3& m);
		void SetUniform(const std::string& name, const wolf::Color4& color);
		void SetUniform(const std::string& name, float val);
		void SetUniform(const std::string& name, const std::vector<glm::mat4>& mats);

		void SetTexture(const std::string& name, const Texture* pTex);

		void SetDepthTest(bool enable) { m_depthTest = enable; }
		void SetDepthWrite(bool enable) { m_depthWrite = enable; }
		void SetDepthFunc(DepthFunc func) { m_depthFunc = func; }

		void SetBlend(bool enable) { m_blendEnabled = enable; }
		void SetBlendMode(BlendMode srcFactor, BlendMode dstFactor) { m_srcFactor = srcFactor; m_dstFactor = dstFactor; }
		void SetBlendEquation(BlendEquation equation) { m_blendEquation = equation; }

		const std::string& GetName() const { return m_name; }
		void SetProgram(const std::string& vsFile, const std::string& fsFile);

		void EnableKeyword(const std::string& keyword);
		void DisableKeyword(const std::string& keyword);
		bool IsKeywordEnabled(const std::string& keyword);
		//-------------------------------------------------------------------------

	private:
		//-------------------------------------------------------------------------
		// PRIVATE METHODS
		//-------------------------------------------------------------------------
		
		// Made private to enforce creation/deletion via MaterialManager
		Material(const std::string& name);
		virtual ~Material();
		
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PRIVATE MEMBERS
		//-------------------------------------------------------------------------
		std::string									m_name;
		Program*									m_pProgram;
		std::map<std::string, Program::Uniform>		m_uniforms;

		bool								m_depthTest;
		DepthFunc							m_depthFunc;
		bool								m_depthWrite;

		bool								m_blendEnabled;
		BlendMode							m_srcFactor;
		BlendMode							m_dstFactor;
		BlendEquation						m_blendEquation;
		std::set<std::string> 				m_keywords;
		std::size_t 						m_currVariant;

		// For caching
		static bool							s_currentDepthTest;
		static bool							s_currentDepthWrite;
		static DepthFunc					s_currentDepthFunc;

		static bool							s_currentBlendEnable;
		static BlendEquation				s_currentBlendEquation;
		static BlendMode					s_currentSrcFactor;
		static BlendMode					s_currentDstFactor;
		//-------------------------------------------------------------------------
};

}

#endif


