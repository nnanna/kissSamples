/*
	Nnanna Kama
	Implementation of barebones class for managing GLSL Shaders
	Handles only one instance of a vertex and fragment shader each
*/


#include "GLSLContainer.h"
#include "RenderResourceFactory.h"

#ifdef TARGET_GL_SHADERS
#include <stdarg.h>
#include "Macros.h"
#include "ErrorNotify.h"
#include "GL/glew.h"

ShaderContext	GLSLContainer::_gGLSLContext	= NULL;


//=================================================================================================================

GLSLContainer::GLSLContainer(void) :	mVertProgram(0), mFragProgram(0), mShaderProgram(0)
{
	
	if(!_gGLSLContext)
	{
		getStaticContext();
	}

	if(!_gGLSLContext)
	{
		throw ErrorNotify("GLSL Activated... NOT!" );
	}

	// check for graphics card version supported @TODO

}



//=================================================================================================================


GLSLContainer::~GLSLContainer(void)
{
	RenderResourceFactory::onShaderDelete(this);

	glDeleteShader( mVertProgram);
	glDeleteShader( mFragProgram);
	glDeleteProgram( mShaderProgram);

}


//=================================================================================================================


kissU32 GLSLContainer::getStaticContext()
{
	if( _gGLSLContext == NULL )
	{
		//init the extensions & set context to one
		glewInit();
		_gGLSLContext = 1;
	}

	return _gGLSLContext;
}



void GLSLContainer::destroyContext()
{
	_gGLSLContext = NULL;
}


void GLSLContainer::destroyProgram(ShaderProgram& program)
{
	glDeleteProgram(program);
}


//=================================================================================================================


void GLSLContainer::glslErrorCallback(void)
{
	VOID_RETURN_IF_NULL ( _gGLSLContext );

	GLenum lastError = glGetError();

	VOID_RETURN_IF_NULL(lastError);

	switch(lastError)
	{
	case GL_INVALID_ENUM:
		printf("GL_INVALID_ENUM: Whatcha lookin' for?\n");
		break;

	case GL_INVALID_OPERATION:
		printf("GL_INVALID_OPERATION: Bad move, son!\n");
		break;

	case GL_INVALID_VALUE:
		printf("This value ain't worth spit\n");
		break;

	case GL_STACK_OVERFLOW:
	case GL_STACK_UNDERFLOW:
	case GL_OUT_OF_MEMORY:
		printf("Memory issues, dawg\n");
		break;
	}
}

//=================================================================================================================
/*
	@todo: check for valid filename paths.
*/

ShaderProgram GLSLContainer::loadProgram(const char *filename, const char* entry, ShaderProfile target )
{
	ShaderProgram shader;

	shader = glCreateShader( target);

	glShaderSource( shader, 1, &filename, NULL);

	glCompileShader(shader);

	// check if shader compiled
	kiss32 compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (shader && compiled)
	{
		if(!mShaderProgram)
		{
			mShaderProgram = glCreateProgram();
		}

		glAttachShader(mShaderProgram, shader);


		glBindAttribLocation(mShaderProgram, SA_POSITION, "in_pos");
		glBindAttribLocation(mShaderProgram, SA_NORMAL, "in_nor");

		glLinkProgram(mShaderProgram);

		if (target == GL_VERTEX_SHADER)
			mVertProgram = shader;
		else if (target == GL_FRAGMENT_SHADER)
			mFragProgram = shader;
	}
	else
	{
		glDeleteShader( shader);
		shader = NULL;
	}

	return shader;
}

//=================================================================================================================

ShaderProgram GLSLContainer::loadProgram(const char* name, const char* vp_filename, const char* fp_filename)
{
	if( strncmp( mName, name, MAX_NAME ) != 0 )
	{
		if(mShaderProgram)
		{
			glDeleteProgram( mShaderProgram);
			mShaderProgram = NULL;
		}

		strcpy_s( mName, MAX_NAME, name );
	}

	if(!mShaderProgram)
	{
		getShaderFromFile(vp_filename, GL_VERTEX_SHADER);
		getShaderFromFile(fp_filename, GL_FRAGMENT_SHADER);
	}
	
	return mShaderProgram;
}

//=================================================================================================================

ShaderProgram GLSLContainer::loadProgram(int numFiles, ...)
{
	// USELESS. ONLY KEPT FOR REFERENCE.
	va_list ap;

	va_start( ap, numFiles);

	for (int ii = 0; ii < numFiles; ii++)
	{
		const char* filename = va_arg( ap, const char*);
		loadProgram(filename, NULL, GL_VERTEX_SHADER);
	}

	return mShaderProgram;
}

char* GLSLContainer::getShaderFromFile(const char* filename, ShaderProfile target)
{
	FILE *shaderFile;
	char *text;
	long size;

	//must read files as binary to prevent problems from newline translation
	kiss32 error = fopen_s( &shaderFile, filename, "rb");

	if ( error != NULL )
		return 0;

	fseek( shaderFile, 0, SEEK_END);

	size = ftell(shaderFile);

	fseek( shaderFile, 0, SEEK_SET);

	text = new char[size+1];

	fread( text, size, 1, shaderFile);

	fclose( shaderFile);

	text[size] = '\0';

	loadProgram(text, NULL, target);

	delete []text;

	return NULL;

}


//=================================================================================================================


void GLSLContainer::bindProgram(ShaderProgram& prog)
{
	glUseProgram(prog);

	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}

void GLSLContainer::unbindProgram()
{
	glUseProgram(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

//=================================================================================================================
/*
	This works with the assumption that the one shader file contains both vp and fp
	@TODO: combine both shaders in loadProgram()
*/


ShaderParameter GLSLContainer::getNamedParam( const char* name )
{
	ParamGLSLMap::iterator itr = mGlslParamMap.find(name);

	
	if( itr == mGlslParamMap.end() )
	{
		ShaderParameter param = glGetUniformLocation( mShaderProgram, name );

		if(param >= 0)
			mGlslParamMap[name] = param;

		return param;
	}	
	else
	{
		return itr->second;
	}

}

//=================================================================================================================


void GLSLContainer::setFloatParameter(ShaderParameter param, float value)
{
	glUniform1f(param, value);
}

void GLSLContainer::setFloatParameter(const char* name, const float value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setFloatParameter(param, value);
	}
}

void GLSLContainer::setVectorParameter(ShaderParameter param, const VECTOR3& value)
{
	glUniform3f(param, value.x, value.y, value.z);
}

void GLSLContainer::setVectorParameter(const char* name, const VECTOR3& value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

void GLSLContainer::setVectorParameter(ShaderParameter param, const float* value)
{
	glUniform3fv(param, 3, value);
}

void GLSLContainer::setVectorParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

void GLSLContainer::setMatrixParameter(ShaderParameter param, const Matrix4x4& value)
{
	glUniformMatrix4fv(param, 16, false, (float*)value.m);
}

void GLSLContainer::setMatrixParameter(const char* name, const Matrix4x4& value)
{
	ShaderParameter param = getNamedParam(name);
	if(param)
	{
		setMatrixParameter(param, value);
	}
}

void GLSLContainer::setMatrixParameter(ShaderParameter param, float* value)
{
	glUniformMatrix4fv(param, 16, false, value);
}

void GLSLContainer::setMatrixParameter(const char* name, float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setMatrixParameter(param, value);
	}
}

void GLSLContainer::setStateMatrixParameter(ShaderParameter param)
{
	//cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
}

void GLSLContainer::getMatrixParameter(ShaderParameter param, float* matrix)
{
	//cgGLGetMatrixParameterfr(param, matrix);
}



//=================================================================================================================

#endif