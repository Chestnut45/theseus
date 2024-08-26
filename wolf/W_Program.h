//-----------------------------------------------------------------------------
// File:			W_Program.h
// Original Author:	Gordon Wood
//
// Class encapsulating a vertex shader and pixel shader combo, and allowing
// easy setting of uniforms on the program
//-----------------------------------------------------------------------------
#ifndef W_PROGRAM_H
#define W_PROGRAM_H

#include "W_Types.h"
#include <string>
#include <set>
#include <map>
#include <vector>

namespace wolf
{
class Texture;

class Program
{
	friend class ProgramManager;
	friend class Material;
	public:
		//-------------------------------------------------------------------------
		// PUBLIC INTERFACE
		//-------------------------------------------------------------------------
		void Bind(std::size_t variantKey = 0, const std::set<std::string>& keywords = std::set<std::string>());

        void SetUniform(const std::string& name, const glm::mat4& m);
        void SetUniform(const std::string& name, const glm::mat3& m);
        void SetUniform(const std::string& name, const glm::vec3& v);
        void SetUniform(const std::string& name, const glm::vec4& v);
		void SetUniform(const std::string& name, const wolf::Color4& color);
		void SetUniform(const std::string& name, int val);
		void SetUniform(const std::string& name, float val);

		static std::size_t GetVariantKey(const std::set<std::string>& keywords);
		//-------------------------------------------------------------------------

	private:
		//-------------------------------------------------------------------------
		// PRIVATE TYPES
		//-------------------------------------------------------------------------
		enum UniformType
		{
			Matrix4 = 0,
			Matrix4Array,
			Matrix3,
			Vector4,
			Vector3,
			Color,
			Int,
			Float,
			Texture,
			Unknown
		};

		struct Uniform
		{
			UniformType type;
			glm::mat4 m4;
			std::vector<glm::mat4> m4Array;
			glm::mat3 m3;
			glm::vec4 v4;
			glm::vec3 v3;
			wolf::Color4 color;
			const wolf::Texture* pTexture;
			int i;
			float f;

			Uniform() : type(UniformType::Unknown), m4(1.0f), m4Array(), m3(1.0), v4(1.0f,1.0f,1.0f,1.0f), v3(1.0f,1.0f,1.0f), color(0.0f,0.0f,0.0f,0.0f), pTexture(nullptr), i(0), f(0.0f) {}
		};
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PRIVATE METHODS
		//-------------------------------------------------------------------------

		// Made private to enforce usage via ProgramManager
		Program(const std::string& vsFile, const std::string& fsFile);
		virtual ~Program();
		
		void _compile(std::size_t variantKey, const std::set<std::string>& keywords);
		bool _compileShader(GLuint* pShader, GLenum shaderType, const std::string& preamble, const std::string& src);
		bool _linkProgram(GLuint prog);
		void _uploadUniforms(std::size_t variantKey);

		void _setUniform(const std::string& name, const Uniform& u);
		//-------------------------------------------------------------------------

		//-------------------------------------------------------------------------
		// PRIVATE MEMBERS
		//-------------------------------------------------------------------------
		typedef std::map<std::string, int> UniformLocationMap;
		std::map<std::size_t, GLuint>				m_glProgramVariants;
		std::map<std::size_t, UniformLocationMap>	m_uniformLocationsPerVariant;
		std::map<std::string, Uniform>				m_uniforms;
		std::string 								m_vsSrc;
		std::string 								m_fsSrc;
		//-------------------------------------------------------------------------
};

}

#endif
