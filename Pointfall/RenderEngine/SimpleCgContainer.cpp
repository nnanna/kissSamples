/*
	Nnanna Kama
	Implementation of barebones class for managing cg Shaders
	Handles only one instance of a vertex and fragment shader each
*/

#include "SimpleShaderContainer.h"


#if TARGET_CG_SHADERS
#include "Macros.h"
#include "ErrorNotify.h"
#include "RenderResourceFactory.h"


ShaderContext	SimpleShaderContainer::_gShaderContext	= NULL;

const char	*myProgramName			= "CRAP_SLAP_TRAP",
			*gLitShaderFilename	= "..\\media\\programs\\basicLight.cgfx",
			*gBasicLitVertProgram	= "C5E1v_basicLight",
			*gBasicLitFragProgram	= "main_fp";


//=================================================================================================================

SimpleShaderContainer::SimpleShaderContainer(void) :	mVertProgram(0), mFragProgram(0), mShaderProgram(0), mIsBound(false)
{	
	if(!_gShaderContext)
	{
		getStaticContext();
		cgSetErrorCallback( SimpleShaderContainer::shaderErrorCallback );
	}

	if(!_gShaderContext)
	{
		throw ErrorNotify("CG Activated... NOT!" );
	}

	mVertProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	mFragProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
}



//=================================================================================================================


SimpleShaderContainer::~SimpleShaderContainer(void)
{
	RenderResourceFactory::onShaderDelete(this);

	if(mVertProgram)	cgDestroyProgram(mVertProgram);
	if(mFragProgram)	cgDestroyProgram(mFragProgram);

}


//=================================================================================================================


ShaderContext SimpleShaderContainer::getStaticContext()
{
	if( _gShaderContext == NULL )
	{
		_gShaderContext = cgCreateContext();
	}

	return _gShaderContext;
}



void SimpleShaderContainer::destroyContext()
{
	if(_gShaderContext)		cgDestroyContext(_gShaderContext);
}


//=================================================================================================================
/*
	This is called everytime Cg encounters an error
	Uncomment exit() to make the program gracefully quit on error
*/

void SimpleShaderContainer::shaderErrorCallback(void)
{
	VOID_RETURN_IF_NULL ( _gShaderContext );

	CGerror lastError = cgGetError();

    if(lastError)
	{
        const char *listing = cgGetLastListing(_gShaderContext);
        printf("%s\n", cgGetErrorString(lastError));
        printf("%s\n", listing);
        //exit(-1);
    }
}

//=================================================================================================================
/*
	@todo: check for valid filename paths.
*/

ShaderProgram SimpleShaderContainer::loadProgram(const char *filename, const char *vpEntry,  const char *fpEntry )
{
	if( strncmp( mName, filename, MAX_NAME ) != 0 )
	{
		if(mShaderProgram)
		{
			cgDestroyProgram( mShaderProgram);
			mShaderProgram = NULL;
		}

		strcpy_s( mName, MAX_NAME, filename );
	}

	if (!mShaderProgram)
	{
		mVertProgram = cgCreateProgramFromFile(getStaticContext(), CG_SOURCE, filename, mVertProfile, vpEntry, NULL);
		mFragProgram = cgCreateProgramFromFile(getStaticContext(), CG_SOURCE, filename, mFragProfile, fpEntry, NULL);

		if(!mVertProgram || !mFragProgram)
		{
			printf("Error creating program '%s'\n", filename);
			return NULL;
		}

		mShaderProgram = cgCombinePrograms2(mVertProgram, mFragProgram);

		cgGLLoadProgram(mVertProgram);
		cgGLLoadProgram(mFragProgram);
		cgGLLoadProgram(mShaderProgram);
	}

	return mShaderProgram;
}


ShaderProgram SimpleShaderContainer::loadProgram(const char *filename, const char *entry, ShaderProfile profile)
{
   
    ShaderProgram program = NULL;

	program = cgCreateProgramFromFile(getStaticContext(), CG_SOURCE, filename, profile, entry, NULL);
    if (!program)
	{
        printf("Error creating program '%s'\n", filename);
		return NULL;
    }
    
	cgGLLoadProgram(program);

	(profile == mVertProfile) ?	mVertProgram = program :
								mFragProgram = program;

	if (mVertProgram && mFragProgram)
	{
		mShaderProgram = cgCombinePrograms2(mVertProgram, mFragProgram);
		cgGLLoadProgram(mShaderProgram);
	}
	else
	{
		mShaderProgram = program;
	}

    return program;
}


void SimpleShaderContainer::updateProgramParameters()
{
	cgUpdateProgramParameters(mVertProgram);
	cgUpdateProgramParameters(mFragProgram);
}

//=================================================================================================================
/*
	This works with the assumption that the one shader file contains both vp and fp
	So any param handles are obtained via the vp and saved in the ParamMap.

	Better still, we could validate the param returned by cgGetNamedParameter via vp
	if invalid, we try obtaining it through the fp... Hold'up, who's we??
*/


ShaderParameter SimpleShaderContainer::getNamedParam( const char* name )
{
	ParamShaderMap::iterator itr = mShaderParamMap.find(name);

	
	if( itr == mShaderParamMap.end() )
	{
		ShaderParameter param = cgGetNamedParameter( mShaderProgram, name );

		if(param)
			mShaderParamMap[name] = param;

		return param;
	}	
	else
	{
		return itr->second;
	}

}

//=================================================================================================================

void SimpleShaderContainer::bindProgram( ShaderProgram& pProg )
{
	cgGLBindProgram(pProg);
}


void SimpleShaderContainer::unbindVertProgram()
{
	cgGLUnbindProgram(mVertProfile);
}

void SimpleShaderContainer::unbindFragProgram()
{
	cgGLUnbindProgram(mFragProfile);
}

//=================================================================================================================

void SimpleShaderContainer::unbindProgram()
{
	unbindVertProgram();
	unbindFragProgram();
	
	mIsBound = false;
}

//=================================================================================================================

void SimpleShaderContainer::enableVertProfile( void )
{
	cgGLEnableProfile(mVertProfile);
}

void SimpleShaderContainer::enableFragProfile( void )
{
	cgGLEnableProfile(mFragProfile);
}

void SimpleShaderContainer::disableVertProfile( void )
{
	cgGLDisableProfile(mVertProfile);
}

void SimpleShaderContainer::disableFragProfile( void )
{
	cgGLDisableProfile(mFragProfile);
}

template<>
void SimpleShaderContainer::setFloatParameter(ShaderParameter param, float value)
{
	cgSetParameter1f(param, value);
}

template<>
void SimpleShaderContainer::setFloatParameter(const char* name, const float value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setFloatParameter(param, value);
	}
}

template<>
void SimpleShaderContainer::setVectorParameter(ShaderParameter param, const float* value)
{
	cgSetParameter3fv(param, value);
}

template<>
void SimpleShaderContainer::setVectorParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

template<>
void SimpleShaderContainer::setVectorParameter(ShaderParameter param, const vec3* value)
{
	cgSetParameter3f(param, value->x, value->y, value->z);
}

template<>
void SimpleShaderContainer::setVectorParameter(const char* name, const vec3* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

template<>
void SimpleShaderContainer::setMatrixParameter(ShaderParameter param, const float* value)
{
	cgSetMatrixParameterfr(param, value);
}

template<>
void SimpleShaderContainer::setMatrixParameter(ShaderParameter param, const Matrix4x4* value)
{
	cgSetMatrixParameterfr(param, (float*)value->m);
}

template<>
void SimpleShaderContainer::setMatrixParameter(const char* name, const Matrix4x4* value)
{
	ShaderParameter param = getNamedParam(name);
	if(param)
	{
		setMatrixParameter(param, value);
	}
}

template<>
void SimpleShaderContainer::setMatrixParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setMatrixParameter(param, value);
	}
}

void SimpleShaderContainer::getMatrixParameter(ShaderParameter param, float* matrix)
{
	cgGLGetMatrixParameterfr(param, matrix);
}



//=================================================================================================================
#endif