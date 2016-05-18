
/*
	Nnanna Kama
	Barebones class for managing GLSL Shaders
	As an alternative to CG shaders which doesn't have any good 3rd-party profiling tools.
	Uses a map to store parameter handles, so it's owning class don't gotta worry about such.
	ALL MATRIX GET & SET accessors presume a row-major format.
*/

#ifndef GLSLCONTAINER_H
#define GLSLCONTAINER_H

#include "ShaderContainer.h"

#ifdef TARGET_GL_SHADERS
#include <map>

typedef enum ShaderAttribute
{
	SA_POSITION		= 0,
	SA_NORMAL
}ShaderAttribute;


typedef std::map<const char*, ShaderParameter >	ParamGLSLMap;


static const char	*myProgramName			= "CRAP_SLAP_TRAP",
					*gDefaultShaderFilename	= "__default",
					*gDefaultVertProgram	= "..\\media\\programs\\basicLight_v.glsl",
					*gDefaultFragProgram	= "..\\media\\programs\\basicLight_f.glsl";



class GLSLContainer : public IShaderContainer
{

public:
	  GLSLContainer(void);

	  ~GLSLContainer(void);
	
	static	ShaderContext	getStaticContext(void);

	static	void			destroyContext(void);

	static	void			destroyProgram(ShaderProgram& program);

	static	void			glslErrorCallback(void);

	
	ShaderContext			getContext()	{ return GLSLContainer::getStaticContext(); }

	ShaderProgram		loadProgram(int numFiles, ...);

	ShaderProgram		loadProgram(const char* name, const char* vp_filename, const char* fp_filename);

	ShaderProgram		loadProgram(const char *filename, const char *entry, ShaderProfile profile);

	ShaderProgram		getVertProgram(void) const	{return mVertProgram;}
	
	ShaderProgram		getFragProgram(void) const	{return mFragProgram;}

	ShaderProfile		getVertProfile(void) const	{return mVertProfile;}

	ShaderProfile		getFragProfile(void) const	{return mFragProfile;}

	char*				getShaderFromFile(const char* filename, ShaderProfile target);

	
// get named vertex and fragment program parameters

	ShaderParameter		getNamedParam( const char* name );



	
	void	enableVertProfile(void)		{};

	void	enableFragProfile(void)		{};

	void	disableVertProfile(void)	{};

	void	disableFragProfile(void)	{};

	void	setFloatParameter(ShaderParameter param, const float value);

	void	setFloatParameter(const char* name,	const float value);

	void	setVectorParameter(ShaderParameter param, const VECTOR3& value);

	void	setVectorParameter(const char* name, const VECTOR3& value);

	void	setVectorParameter(ShaderParameter param, const float* value);

	void	setVectorParameter(const char* name, const float* value);

	void	setMatrixParameter(ShaderParameter param, const Matrix4x4& value);

	void	setMatrixParameter(const char* name, const Matrix4x4& value);

	void	setMatrixParameter(ShaderParameter param, float* value);

	void	setMatrixParameter(const char* name, float* value);

	void	setStateMatrixParameter(ShaderParameter param);

	void	updateProgramParameters()	{};

	void	getMatrixParameter(ShaderParameter, float*);

	void	bindVertProgram()	{	bindProgram(mVertProgram); }

	void	bindFragProgram()	{	bindProgram(mFragProgram);	}

	void	bindProgram()		{	bindProgram(mShaderProgram); }

	void	bindProgram(ShaderProgram& prog);

	void	unbindProgram();

	void	unbindVertProgram(void)	{ unbindProgram(); }

	void	unbindFragProgram(void)	{ unbindProgram(); }


private:

	static   ShaderContext	_gGLSLContext;

	ShaderProfile			mVertProfile;			//for vertex shader

	ShaderProfile			mFragProfile;			//for fragment shader
	
	ShaderProfile			mVertProgram;

	ShaderProfile			mFragProgram;

	ShaderProgram			mShaderProgram;

	ParamGLSLMap			mGlslParamMap;

	char					mName[MAX_NAME];
};

#endif	//TARGET_GL_SHADERS


#endif