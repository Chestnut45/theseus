//-----------------------------------------------------------------------------
// File:			W_Program.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_Program.h"
#include "W_Common.h"
#include "W_Texture.h"

namespace wolf
{

static const char* gs_aAttributeMap[wolf::AT_NUM_ATTRIBS] =
{
	"a_position",	//AT_Position = 0,
	"a_color",		//AT_Color,
	"a_uv1",		//AT_TexCoord1	
	"a_uv2",		//AT_TexCoord2	
	"a_uv3",		//AT_TexCoord3	
	"a_uv4",		//AT_TexCoord4	
	"a_uv5",		//AT_TexCoord5	
	"a_uv6",		//AT_TexCoord6	
	"a_uv7",		//AT_TexCoord7	
	"a_uv8",		//AT_TexCoord8	
	"a_normal",		//AT_Normal
	"a_tangent",	//AT_Tangent
	"a_bitangent",	//AT_Tangent
	"a_boneIndices",//AT_BoneIndices
	"a_boneWeights",//AT_BoneWeights
};

//----------------------------------------------------------
// Constructor
//----------------------------------------------------------
Program::Program(const std::string& vsFile, const std::string& fsFile) 
{
    GLchar* pFileContents = (GLchar *)wolf::LoadWholeFile(vsFile);

    if (pFileContents)
        m_vsSrc = pFileContents;
    else
        printf("Failed to load file %s\n", vsFile.c_str());

    pFileContents = (GLchar *)wolf::LoadWholeFile(fsFile);

    if (pFileContents)
        m_fsSrc = pFileContents;
    else
        printf("Failed to load file %s\n", fsFile.c_str());
}

//----------------------------------------------------------
// Destructor
//----------------------------------------------------------
Program::~Program()
{
    for(auto it = m_glProgramVariants.begin(); it != m_glProgramVariants.end(); ++it)
    {
		glDeleteProgram(it->second);
    }
}

//----------------------------------------------------------
// Binds this program as the current one
//----------------------------------------------------------
void Program::Bind(std::size_t variantKey, const std::set<std::string>& keywords)
{
    if(m_glProgramVariants.find(variantKey) == m_glProgramVariants.end())
        _compile(variantKey, keywords);

	glUseProgram(m_glProgramVariants[variantKey]);
    _uploadUniforms(variantKey);
}

//----------------------------------------------------------
// Sets a matrix uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, const glm::mat4& m)
{
    m_uniforms[name].m4 = m;
    m_uniforms[name].type = UniformType::Matrix4;
}

//----------------------------------------------------------
// Sets a matrix uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, const glm::mat3& m)
{
    m_uniforms[name].m3 = m;
    m_uniforms[name].type = UniformType::Matrix3;
}

//----------------------------------------------------------
// Sets a vector uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, const glm::vec3& v)
{
    m_uniforms[name].v3 = v;
    m_uniforms[name].type = UniformType::Vector3;
}

//----------------------------------------------------------
// Sets a vector uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, const glm::vec4& v)
{
    m_uniforms[name].v4 = v;
    m_uniforms[name].type = UniformType::Vector4;
}

//----------------------------------------------------------
// Sets a color uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, const wolf::Color4& color)
{
    m_uniforms[name].color = color;
    m_uniforms[name].type = UniformType::Color;
}

//----------------------------------------------------------
// Sets an integer uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, int val)
{
    m_uniforms[name].i = val;
    m_uniforms[name].type = UniformType::Int;
}

//----------------------------------------------------------
// Sets a floating point uniform of the given name
//----------------------------------------------------------
void Program::SetUniform(const std::string& name, float val)
{
    m_uniforms[name].f = val;
    m_uniforms[name].type = UniformType::Float;
}

void Program::_setUniform(const std::string& name, const Uniform& u)
{
    m_uniforms[name] = u;
}

//----------------------------------------------------------
// Compiles the shader of the given type, returning true
// on success, else false. Shader object stored in pShader
// on success
//----------------------------------------------------------
bool Program::_compileShader(GLuint* pShader, GLenum shaderType, const std::string& preamble, const std::string& src)
{
    GLint iStatus;
    const GLchar* sources[] = 
    {
        preamble.c_str(),
        src.c_str()
    };

    *pShader = glCreateShader(shaderType);
    glShaderSource(*pShader, 2, sources, NULL);
    glCompileShader(*pShader);

    GLint iLogLength;
    glGetShaderiv(*pShader, GL_INFO_LOG_LENGTH, &iLogLength);
    if (iLogLength > 0)
    {
        GLchar *pLog = (GLchar *)malloc(iLogLength);
        glGetShaderInfoLog(*pShader, iLogLength, &iLogLength, pLog);
        printf("Shader compile log:\n%s\n", pLog);
        free(pLog);
    }

    glGetShaderiv(*pShader, GL_COMPILE_STATUS, &iStatus);
    if (iStatus == 0)
    {
        glDeleteShader(*pShader);
        return false;
    }

    return true;
}

//----------------------------------------------------------
// Performs the linking stage of the vertex and pixel shader,
// resulting in a final program object to use. Checks for
// errors and prints them and returns true on success, else false
//----------------------------------------------------------
bool Program::_linkProgram(GLuint prog)
{
    GLint iStatus;
    
    glLinkProgram(prog);
    
    GLint iLogLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &iLogLength);
    if (iLogLength > 0)
    {
        GLchar* pLog = (GLchar *)malloc(iLogLength);
        glGetProgramInfoLog(prog, iLogLength, &iLogLength, pLog);
        printf("Program link log:\n%s\n", pLog);
        free(pLog);
    }
    
    glGetProgramiv(prog, GL_LINK_STATUS, &iStatus);
    if (iStatus == 0)
        return false;
    
    return true;
}

void Program::_compile(std::size_t variantKey, const std::set<std::string>& keywords)
{
    if(m_glProgramVariants.find(variantKey) != m_glProgramVariants.end())
        return; // already built this variant

    GLuint uiVS, uiPS;
    
    // Construct the "preamble" which is our version string + all the keywords as #defines
    std::string preamble = "#version 150\n";
    for(auto it = keywords.begin(); it != keywords.end(); ++it)
    {
        preamble += std::string("#define ") + *it + "\n";
    }


    if( !_compileShader(&uiVS, GL_VERTEX_SHADER, preamble, m_vsSrc))
	{
        printf("Failed to compile vertex shader\n");
        return;
    }
    
    if( !_compileShader(&uiPS, GL_FRAGMENT_SHADER, preamble, m_fsSrc))
	{
        printf("Failed to compile pixel shader\n");
		glDeleteShader(uiVS);
        return;
    }

    GLuint glProgram = glCreateProgram();
    
    glAttachShader(glProgram, uiVS);
    glAttachShader(glProgram, uiPS);
    
	for( int i = 0; i < wolf::AT_NUM_ATTRIBS; i++ )
	{
		glBindAttribLocation(glProgram, i, gs_aAttributeMap[i]);
	}
    
    if( !_linkProgram(glProgram))
    {
        printf("Failed to link program\n");
        
        if( uiVS )
        {
            glDeleteShader(uiVS);
        }
        if( uiPS )
        {
            glDeleteShader(uiPS);
        }
        if( glProgram )
        {
            glDeleteProgram(glProgram);
			glProgram = 0;
        }
        return;
    }
    
    // Release vertex and fragment shaders.
    if( uiVS )
        glDeleteShader(uiVS);
    if( uiPS )
        glDeleteShader(uiPS);

    m_glProgramVariants[variantKey] = glProgram;
}

void Program::_uploadUniforms(std::size_t variantKey)
{
    GLuint prog = m_glProgramVariants[variantKey];

	int texUnit = 0;
    for(auto it = m_uniforms.begin(); it != m_uniforms.end(); ++it)
    {
        UniformLocationMap& locations = m_uniformLocationsPerVariant[variantKey];

        if(locations.find(it->first) == locations.end())
        {
            int loc = glGetUniformLocation(prog,it->first.c_str());
            if( loc == -1 )
                continue;
            locations[it->first] = loc;
        }

        switch(it->second.type)
        {
            case UniformType::Matrix4:
                glUniformMatrix4fv(locations[it->first], 1, GL_FALSE, glm::value_ptr(it->second.m4));
                break;
            case UniformType::Matrix4Array:
                if(it->second.m4Array.size() > 0)
                    glUniformMatrix4fv(locations[it->first], (GLint)it->second.m4Array.size(), GL_FALSE, glm::value_ptr(it->second.m4Array[0]));
                break;
            case UniformType::Matrix3:
                glUniformMatrix3fv(locations[it->first], 1, GL_FALSE, glm::value_ptr(it->second.m3));
                break;
            case UniformType::Vector3:
                glUniform3fv(locations[it->first], 1, glm::value_ptr(it->second.v3));
                break;
            case UniformType::Vector4:
                glUniform4fv(locations[it->first], 1, glm::value_ptr(it->second.v4));
                break;
            case UniformType::Color:
                glUniform4fv(locations[it->first], 1, (GLfloat*)&it->second.color);
                break;
            case UniformType::Int:
                glUniform1i(locations[it->first], it->second.i);
                break;
            case UniformType::Float:
                glUniform1f(locations[it->first], it->second.f);
                break;
			case UniformType::Texture:
				// If it's a texture, we need to do a bit more work as we need to keep
				// track of how many texture units we need and switch to them, and then
				// associate that unit number with the uniform
				it->second.pTexture->Bind(texUnit);
                glUniform1i(locations[it->first], texUnit);
				texUnit++;
                break;
            default:
                break;
        }
    }
}

std::size_t Program::GetVariantKey(const std::set<std::string>& keywords)
{
    std::size_t key = 0;
    for(auto it = keywords.begin(); it != keywords.end(); ++it)
    {
        #if defined __APPLE__
            key += std::hash<std::string>()(*it);
        #else
            key += std::hash<std::string>{}(*it);
        #endif
    }
    return key;
}

}
