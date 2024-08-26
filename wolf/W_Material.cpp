//-----------------------------------------------------------------------------
// File:			W_Material.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_Material.h"
#include "W_Common.h"

namespace wolf
{
bool			Material::s_currentDepthTest = false;
bool			Material::s_currentDepthWrite = true;
DepthFunc		Material::s_currentDepthFunc = DF_Less;
bool			Material::s_currentBlendEnable = false;
BlendEquation	Material::s_currentBlendEquation = BE_Add;
BlendMode		Material::s_currentSrcFactor = BM_One;
BlendMode		Material::s_currentDstFactor = BM_Zero;

static GLenum g_aDepthFuncMap[DF_NUM_DEPTH_FUNCS] =
{
	GL_NEVER,			//DF_Never,
	GL_LESS,			//DF_Less,
	GL_LEQUAL,			//DF_LessEqual,
	GL_EQUAL,			//DF_Equal,
	GL_GREATER,			//DF_Greater,
	GL_GEQUAL,			//DF_GreaterEqual,
	GL_NOTEQUAL,		//DF_NotEqual,
	GL_ALWAYS,			//DF_Always,
};

static GLenum g_aBlendEquationMap[BE_NUM_BLEND_EQUATIONS] =
{
	GL_FUNC_ADD,				//BE_Add
	GL_FUNC_SUBTRACT,			//BE_Subtract
	GL_FUNC_REVERSE_SUBTRACT	//BE_ReverseSubtract
};

static GLenum g_aBlendModeMap[] =
{
	GL_SRC_ALPHA,				//BM_SrcAlpha,		
	GL_ONE,						//BM_One,				
	GL_SRC_COLOR,				//BM_SrcColor,
	GL_ONE_MINUS_SRC_COLOR,		//BM_OneMinusSrcColor,	
	GL_ONE_MINUS_SRC_ALPHA,		//BM_OneMinusSrcAlpha,
	GL_DST_ALPHA,				//BM_DstAlpha,			
	GL_ONE_MINUS_DST_ALPHA,		//BM_OneMinusDstAlpha,
	GL_DST_COLOR,				//BM_DstColor,			
	GL_ONE_MINUS_DST_COLOR,		//BM_OneMinusDstColor,	
	GL_ZERO,					//BM_Zero,				
};

//----------------------------------------------------------
// Constructor
//----------------------------------------------------------
Material::Material(const std::string& name) 
	: m_name(name), m_pProgram(0),
	  m_depthTest(true), m_depthWrite(true), m_depthFunc(DF_LessEqual),
	  m_blendEnabled(false), m_srcFactor(BM_One), m_dstFactor(BM_Zero), m_blendEquation(BE_Add),
	  m_currVariant(0)
{
}

//----------------------------------------------------------
// Destructor
//----------------------------------------------------------
Material::~Material()
{
}

//----------------------------------------------------------
// Sets the whole pipeline up for this material
//----------------------------------------------------------
void Material::Apply()
{
	// Set all the uniforms we have
	for( auto iter = m_uniforms.begin(); iter != m_uniforms.end(); iter++ )
	{
		m_pProgram->_setUniform(iter->first, iter->second);
	}

	// Bind the shader after setting all its uniforms
	m_pProgram->Bind(m_currVariant, m_keywords);

	// Set up depth test
	{
		if( m_depthTest != s_currentDepthTest )
		{
			s_currentDepthTest = m_depthTest;

			if( m_depthTest )
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}

		if( m_depthTest )
		{
			if( m_depthFunc != s_currentDepthFunc )
			{
				s_currentDepthFunc = m_depthFunc;

				glDepthFunc(g_aDepthFuncMap[m_depthFunc]);
			}

			if( m_depthWrite != s_currentDepthWrite )
			{
				s_currentDepthWrite = m_depthWrite;

				glDepthMask( m_depthWrite );
			}
		}
	}

	// Set up blending
	{
		if( m_blendEnabled != s_currentBlendEnable )
		{
			s_currentBlendEnable = m_blendEnabled;

			if( m_blendEnabled )
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);
        }
        
        if( m_blendEnabled )
        {
			if( m_blendEquation != s_currentBlendEquation )
			{
				s_currentBlendEquation = m_blendEquation;
				glBlendEquation(g_aBlendEquationMap[m_blendEquation]);
			}

			if( m_srcFactor != s_currentSrcFactor || m_dstFactor != s_currentDstFactor )
			{
				s_currentSrcFactor = m_srcFactor;
				s_currentDstFactor = m_dstFactor;
				glBlendFunc(g_aBlendModeMap[m_srcFactor], g_aBlendModeMap[m_dstFactor]);
			}
		}
	}
}

//----------------------------------------------------------
// Sets what program this material uses
//----------------------------------------------------------
void Material::SetProgram(const std::string& vsFile, const std::string& fsFile)
{
	if( m_pProgram )
		ProgramManager::DestroyProgram(m_pProgram);

	m_pProgram = ProgramManager::CreateProgram(vsFile, fsFile);
}

//----------------------------------------------------------
// Set uniform version for mat4 uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const glm::mat4& m)
{
	m_uniforms[name].type = Program::UniformType::Matrix4;
	m_uniforms[name].m4 = m;
}

//----------------------------------------------------------
// Set uniform version for an array of mat4s 
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const std::vector<glm::mat4>& mats)
{
	m_uniforms[name].type = Program::UniformType::Matrix4Array;
	m_uniforms[name].m4Array = mats;
}

//----------------------------------------------------------
// Set uniform version for mat3 uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const glm::mat3& m)
{
	m_uniforms[name].type = Program::UniformType::Matrix4;
	m_uniforms[name].m3 = m;
}

//----------------------------------------------------------
// Set uniform version for vec4 uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const glm::vec4& v)
{
	m_uniforms[name].type = Program::UniformType::Vector4;
	m_uniforms[name].v4 = v;
}

//----------------------------------------------------------
// Set uniform version for vec4 uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const glm::vec3& v)
{
	m_uniforms[name].type = Program::UniformType::Vector3;
	m_uniforms[name].v3 = v;
}

//----------------------------------------------------------
// Set uniform version for vec4 uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, const wolf::Color4& color)
{
	m_uniforms[name].type = Program::UniformType::Color;
	m_uniforms[name].color = color;
}

//----------------------------------------------------------
// Set uniform version for float uniforms
//----------------------------------------------------------
void Material::SetUniform(const std::string& name, float val)
{
	m_uniforms[name].type = Program::UniformType::Float;
	m_uniforms[name].f = val;
}

//----------------------------------------------------------
// Used to set textures on the material, and associate them
// with names in the shader
//----------------------------------------------------------
void Material::SetTexture(const std::string& name, const Texture* pTex)
{
	m_uniforms[name].type = Program::UniformType::Texture;
	m_uniforms[name].pTexture = pTex;
}

void Material::EnableKeyword(const std::string& keyword)
{
    m_keywords.insert(keyword);
    m_currVariant = Program::GetVariantKey(m_keywords);
}

void Material::DisableKeyword(const std::string& keyword)
{
    m_keywords.erase(keyword);
    m_currVariant = Program::GetVariantKey(m_keywords);
}

bool Material::IsKeywordEnabled(const std::string& keyword)
{
    return m_keywords.find(keyword) != m_keywords.end();
}

}


