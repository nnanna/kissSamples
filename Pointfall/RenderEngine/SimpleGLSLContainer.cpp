/*
	Nnanna Kama
	Implementation of barebones class for managing GLSL Shaders
	Handles only one instance of a vertex and fragment shader each
*/


#include "SimpleShaderContainer.h"

#ifdef TARGET_GL_SHADERS
#include "RenderResourceFactory.h"
#include <stdarg.h>
#include "Macros.h"
#include "ErrorNotify.h"
#include "GL/glew.h"
#include "crc32.h"

typedef ks::vec3		vec3;
typedef ks::Matrix4x4	Matrix;

ShaderContext	SimpleShaderContainer::_gShaderContext	= NULL;

const char	*myProgramName			= "CRAP_SLAP_TRAP",
			*gLitShaderFilename	= "__default",
			*gBasicLitVertProgram	= "..\\media\\programs\\basicLight_v.glsl",
			*gBasicLitFragProgram	= "..\\media\\programs\\basicLight_f.glsl",
			*gUnlitShaderFilename	= "__unlit",
			*gUnlitVertProgram		= "..\\media\\programs\\basicUnlit_v.glsl",
			*gUnlitFragProgram		= "..\\media\\programs\\basicUnlit_f.glsl";



//=================================================================================================================

SimpleShaderContainer::SimpleShaderContainer(void) :	mVertProgram(0), mFragProgram(0), mShaderProgram(0), mIsBound(false)
{
	
	if(!_gShaderContext)
	{
		getStaticContext();
	}

	if(!_gShaderContext)
	{
		throw ErrorNotify("GLSL Activated... NOT!" );
	}

	// check for graphics card version supported @TODO

}



//=================================================================================================================


SimpleShaderContainer::~SimpleShaderContainer(void)
{
	RenderResourceFactory::onShaderDelete(this);

	glDeleteShader( mVertProgram);
	glDeleteShader( mFragProgram);
	glDeleteProgram( mShaderProgram);

}


//=================================================================================================================


ksU32 SimpleShaderContainer::getStaticContext()
{
	if( _gShaderContext == NULL )
	{
		//init the extensions & set context to one
		glewInit();
		_gShaderContext = 1;
	}

	return _gShaderContext;
}



void SimpleShaderContainer::destroyContext()
{
	_gShaderContext = NULL;
}


void SimpleShaderContainer::destroyProgram(ShaderProgram& program)
{
	glDeleteProgram(program);
}


//=================================================================================================================


void SimpleShaderContainer::shaderErrorCallback(void)
{
	VOID_RETURN_IF_NULL ( _gShaderContext );

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

	default:
		throw ErrorNotify("Unknown shiz be going down, boy!");
		break;
	}
}



void SimpleShaderContainer::enableVertProfile(void)		{}

void SimpleShaderContainer::enableFragProfile(void)		{}

void SimpleShaderContainer::disableVertProfile(void)	{}

void SimpleShaderContainer::disableFragProfile(void)	{}

void SimpleShaderContainer::updateProgramParameters()	{}

void SimpleShaderContainer::unbindVertProgram(void)		{}

void SimpleShaderContainer::unbindFragProgram(void)		{}

//=================================================================================================================
/*
	@todo: check for valid filename paths.
*/

ShaderProgram SimpleShaderContainer::loadProgram(const char *filename, const char* entry, ShaderProfile target )
{
	int compiled(0), linked(1);
	ShaderProgram shader = glCreateShader( target );

	glShaderSource( shader, 1, &filename, NULL);

	glCompileShader(shader);

	// check if shader compiled
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (shader && compiled)
	{
		if (target == GL_VERTEX_SHADER)
			mVertProgram = shader;
		else if (target == GL_FRAGMENT_SHADER)
			mFragProgram = shader;
	}
	
	if (mVertProgram && mFragProgram)
	{
		if (mShaderProgram)
			destroyProgram(mShaderProgram);

		mShaderProgram = glCreateProgram();

		glAttachShader(mShaderProgram, mVertProgram);
		glAttachShader(mShaderProgram, mFragProgram);
		// attrib locations have to be bound before linking, else weird shiz happens.
		// and it'll take you days before you even figure out why :(
		glBindAttribLocation(mShaderProgram, SA_POSITION, SP_POS);
		glBindAttribLocation(mShaderProgram, SA_NORMAL, SP_NOR);
		CHECK_GL_ERROR;
		glLinkProgram(mShaderProgram);
		CHECK_GL_ERROR;
		glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &linked);
		CHECK_GL_ERROR;
	}
	
	if ( !compiled || (mShaderProgram && !linked) )
	{
		int log_len(0);
		char log[512];
		if (compiled)
			glGetProgramInfoLog(mShaderProgram, sizeof(log), &log_len, log);		// it's a linking error
		else
			glGetShaderInfoLog(shader, sizeof(log), &log_len, log);

		log_len ? throw ErrorNotify( log ) : throw ErrorNotify("failed to compile or link a shader");
		glDeleteShader( shader);
		glDeleteProgram(mShaderProgram);
		mShaderProgram = NULL;
	}

	return shader;
}

//=================================================================================================================

ShaderProgram SimpleShaderContainer::loadProgram(const char* name, const char* vp_filename, const char* fp_filename)
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

ShaderProgram SimpleShaderContainer::loadProgram(int numFiles, ...)
{
	// USELESS. ONLY KEPT FOR REFERENCE.
	va_list ap;

	va_start( ap, numFiles);

	for (int ii = 0; ii < numFiles; ++ii)
	{
		const char* filename = va_arg( ap, const char*);
		loadProgram(filename, nullptr, GL_VERTEX_SHADER);
	}

	return mShaderProgram;
}

char* SimpleShaderContainer::getShaderFromFile(const char* filename, ShaderProfile target)
{
	FILE *shaderFile;

	//must read files as binary to prevent problems from newline translation
	int error = fopen_s( &shaderFile, filename, "rb");

	if (error != 0)
	{
		ErrorNotify("file load failure");
		return nullptr;
	}

	fseek( shaderFile, 0, SEEK_END);

	long size = ftell(shaderFile);

	fseek( shaderFile, 0, SEEK_SET);

	char* text = new char[size+1];

	fread( text, size, 1, shaderFile);

	fclose( shaderFile);

	text[size] = '\0';

	loadProgram(text, nullptr, target);

	delete []text;

	return nullptr;

}

//=================================================================================================================


void SimpleShaderContainer::bindProgram(ShaderProgram& prog)
{
	CHECK_GL_ERROR;
	glUseProgram(prog);
	glEnableVertexAttribArray(SA_POSITION);
	CHECK_GL_ERROR;
}

void SimpleShaderContainer::unbindProgram()
{
	glUseProgram(0);
	glDisableVertexAttribArray(SA_POSITION);
	glDisableVertexAttribArray(SA_NORMAL);
	glDisableClientState(GL_NORMAL_ARRAY);

	mIsBound = false;
}

//=================================================================================================================
/*
	This works with the assumption that the one shader file contains both vp and fp
*/

ShaderParameter SimpleShaderContainer::getNamedParam(const char* name)
{
	return getNamedParam( ShaderKey( CRC32(name), name ) );
}

ShaderParameter SimpleShaderContainer::getNamedParam( const ShaderKey& pKey )
{
	ParamShaderMap::iterator itr = mShaderParamMap.find(pKey.key);

	if( itr == mShaderParamMap.end() )
	{
		ShaderParameter param = glGetUniformLocation( mShaderProgram, pKey.name );

		if(param >= 0)
			mShaderParamMap[pKey.key] = param;

		return param;
	}	
	else
	{
		return itr->second;
	}

}

//=================================================================================================================


template<>
void SimpleShaderContainer::setFloatParameter(ShaderParameter param, float value)
{
	glUniform1f(param, value);
}

template<>
void SimpleShaderContainer::setFloatParameter(const char* name, const float value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setFloatParameter(param, value);
}

template<>
void SimpleShaderContainer::setFloatParameter(ShaderKey name, const float value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setFloatParameter(param, value);
}


template<>
void SimpleShaderContainer::setVectorParameter(ShaderParameter param, const float* value)
{
	glUniform3fv(param, 1, value);
}

template<>
void SimpleShaderContainer::setVectorParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setVectorParameter(param, value);
}

template<>
void SimpleShaderContainer::setVectorParameter(ShaderKey name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setVectorParameter(param, value);
}

template<>
void SimpleShaderContainer::setVectorParameter(ShaderParameter param, const vec3* value)
{
	glUniform3f(param, value->x, value->y, value->z);
}

template<>
void SimpleShaderContainer::setVectorParameter(const char* name, const vec3* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setVectorParameter(param, value);
}

template<>
void SimpleShaderContainer::setVectorParameter(ShaderKey name, const vec3* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setVectorParameter(param, value);
}

template<>
void SimpleShaderContainer::setMatrixParameter(ShaderParameter param, const float* value)
{
	glUniformMatrix4fv(param, 1, false, value);
}


template<>
void SimpleShaderContainer::setMatrixParameter(ShaderParameter param, const Matrix* value)
{
	setMatrixParameter(param, (float*)value->m);
}

template<>
void SimpleShaderContainer::setMatrixParameter(const char* name, const Matrix* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setMatrixParameter(param, value);
}

template<>
void SimpleShaderContainer::setMatrixParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setMatrixParameter(param, value);
}

template<>
void SimpleShaderContainer::setMatrixParameter(ShaderKey name, const Matrix* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setMatrixParameter(param, value);
}

template<>
void SimpleShaderContainer::setMatrixParameter(ShaderKey name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param >= 0)
		setMatrixParameter(param, value);
}

void SimpleShaderContainer::getMatrixParameter(ShaderParameter param, float* matrix)
{
	//cgGLGetMatrixParameterfr(param, matrix);
}



//=================================================================================================================

#endif