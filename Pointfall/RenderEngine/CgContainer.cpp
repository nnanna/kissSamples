/*
	Nnanna Kama
	Implementation of barebones class for managing cg Shaders
	Handles only one instance of a vertex and fragment shader each
*/

#include "Macros.h"
#include "CgContainer.h"
#include "ErrorNotify.h"

#include "RenderResourceFactory.h"


#ifdef TARGET_CG_SHADERS

ShaderContext	CgContainer::_gCgContext	= NULL;


//=================================================================================================================

CgContainer::CgContainer(void) :	mVertProgram(0), mFragProgram(0), mShaderProgram(0)
{
	
	if(!_gCgContext)
	{
		getStaticContext();
		cgSetErrorCallback( CgContainer::cgErrorCallback );
	}

	if(!_gCgContext)
	{
		throw ErrorNotify("CG Activated... NOT!" );
	}

	mVertProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	mFragProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

}



//=================================================================================================================


CgContainer::~CgContainer(void)
{
	RenderResourceFactory::onShaderDelete(this);

	if(mVertProgram)	cgDestroyProgram(mVertProgram);
	if(mFragProgram)	cgDestroyProgram(mFragProgram);

}


//=================================================================================================================


ShaderContext CgContainer::getStaticContext()
{
	if( _gCgContext == NULL )
	{
		_gCgContext = cgCreateContext();
	}

	return _gCgContext;
}



void CgContainer::destroyContext()
{
	if(_gCgContext)		cgDestroyContext(_gCgContext);
}


//=================================================================================================================
/*
	This is called everytime Cg encounters an error
	Uncomment exit() to make the program gracefully quit on error
*/

void CgContainer::cgErrorCallback(void)
{
	VOID_RETURN_IF_NULL ( _gCgContext );

	CGerror lastError = cgGetError();

    if(lastError)
	{
        const char *listing = cgGetLastListing(_gCgContext);
        printf("%s\n", cgGetErrorString(lastError));
        printf("%s\n", listing);
        //exit(-1);
    }
}

//=================================================================================================================
/*
	@todo: check for valid filename paths.
*/

ShaderProgram CgContainer::loadProgram(const char *filename, const char *vpEntry,  const char *fpEntry )
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


ShaderProgram CgContainer::loadProgram(const char *filename, const char *entry, ShaderProfile profile)
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


void CgContainer::updateProgramParameters()
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


ShaderParameter CgContainer::getNamedParam( const char* name )
{
	ParamCgMap::iterator itr = mCgParamMap.find(name);

	
	if( itr == mCgParamMap.end() )
	{
		ShaderParameter param = cgGetNamedParameter( mShaderProgram, name );

		if(param)
			mCgParamMap[name] = param;		//insert( std::pair<const char*, ShaderParameter>( name, param) );

		return param;
	}
	
	else
	{
		return itr->second;
	}

}

//=================================================================================================================

void CgContainer::enableVertProfile( void )
{
	cgGLEnableProfile(mVertProfile);
}

void CgContainer::enableFragProfile( void )
{
	cgGLEnableProfile(mFragProfile);
}

void CgContainer::disableVertProfile( void )
{
	cgGLDisableProfile(mVertProfile);
}

void CgContainer::disableFragProfile( void )
{
	cgGLDisableProfile(mFragProfile);
}

void CgContainer::setFloatParameter(ShaderParameter param, float value)
{
	cgSetParameter1f(param, value);
}

void CgContainer::setFloatParameter(const char* name, const float value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setFloatParameter(param, value);
	}
}

void CgContainer::setVectorParameter(const char* name, const float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

void CgContainer::setVectorParameter(ShaderParameter param, const float* value)
{
	cgSetParameter3fv(param, value);
}

void CgContainer::setVectorParameter(ShaderParameter param, const VECTOR3& value)
{
	cgSetParameter3f(param, value.x, value.y, value.z);
}

void CgContainer::setVectorParameter(const char* name, const VECTOR3& value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setVectorParameter(param, value);
	}
}

void CgContainer::setMatrixParameter(ShaderParameter param, const Matrix4x4& value)
{
	cgSetMatrixParameterfr(param, (float*)value.m);
}

void CgContainer::setMatrixParameter(const char* name, const Matrix4x4& value)
{
	ShaderParameter param = getNamedParam(name);
	if(param)
	{
		setMatrixParameter(param, value);
	}
}

void CgContainer::setMatrixParameter(const char* name, float* value)
{
	ShaderParameter param = getNamedParam(name);
	if (param)
	{
		setMatrixParameter(param, value);
	}
}

void CgContainer::setMatrixParameter(ShaderParameter param, float* value)
{
	cgSetMatrixParameterfr(param, value);
}

void CgContainer::setStateMatrixParameter(ShaderParameter param)
{
	cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
}

void CgContainer::getMatrixParameter(ShaderParameter param, float* matrix)
{
	cgGLGetMatrixParameterfr(param, matrix);
}



//=================================================================================================================
#endif