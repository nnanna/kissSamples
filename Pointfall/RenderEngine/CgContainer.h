
/*
	Nnanna Kama
	Barebones class for managing cg Shaders
	Contains only one (global) ShaderContext, and single CgProfiles (vertex & fragment).
	Uses a map to store cg parameter handles, so it's owning class don't gotta worry about such.
	ALL MATRIX GET & SET accessors presume a row-major format.
*/

#ifndef CGCONTAINER_H
#define CGCONTAINER_H

#include "ShaderContainer.h"

#ifdef TARGET_CG_SHADERS

#include <map>

typedef std::map<const char*, ShaderParameter >	ParamCgMap;



static const char	*myProgramName			= "CRAP_SLAP_TRAP",
					*gDefaultShaderFilename	= "..\\media\\programs\\basicLight.cgfx",
					*gDefaultVertProgram	= "C5E1v_basicLight",
					*gDefaultFragProgram	= "main_fp";



class CgContainer : public IShaderContainer
{

public:
	  CgContainer(void);

	  ~CgContainer(void);
	
	static	ShaderContext	getStaticContext(void);

	static	void		destroyContext(void);

	static	void		destroyContext(ShaderContext context)	{cgDestroyContext(context);}

	static	void		destroyProgram(ShaderProgram program)	{cgDestroyProgram(program);}

	static	void		cgErrorCallback(void);


	ShaderContext		getContext()	{ return CgContainer::getStaticContext(); }

	ShaderProgram		loadProgram(const char* filename, const char* vertEntry, const char* fragEntry);

	ShaderProgram		loadProgram(const char *filename, const char *entry, ShaderProfile profile);

	ShaderProgram		getVertProgram(void) const	{return mVertProgram;}
	
	ShaderProgram		getFragProgram(void) const	{return mFragProgram;}

	ShaderProfile		getVertProfile(void) const	{return mVertProfile;}

	ShaderProfile		getFragProfile(void) const	{return mFragProfile;}

	
// get named vertex and fragment program parameters

	ShaderParameter		getNamedParam( const char* name );



	
	void	enableVertProfile(void);

	void	enableFragProfile(void);

	void	disableVertProfile(void);

	void	disableFragProfile(void);

	void	setFloatParameter(ShaderParameter param, const float value);

	void	setFloatParameter(const char* name,	const float value);

	void	setVectorParameter(ShaderParameter param, const VECTOR3& value);

	void	setVectorParameter(ShaderParameter param, const float* value);

	void	setVectorParameter(const char* name, const float* value);

	void	setVectorParameter(const char* name, const VECTOR3& value);

	void	setMatrixParameter(ShaderParameter param, const Matrix4x4& value);

	void	setMatrixParameter(const char* name, const Matrix4x4& value);

	void	setMatrixParameter(ShaderParameter param, float* value);

	void	setMatrixParameter(const char* name, float* value);

	void	setStateMatrixParameter(ShaderParameter);

	void	updateProgramParameters();

	void	getMatrixParameter(ShaderParameter, float*);

	void	bindProgram()			{	cgGLBindProgram(mShaderProgram); }

	void	unbindProgram()			{	unbindVertProgram(); unbindFragProgram(); }

	void	bindVertProgram()		{	cgGLBindProgram(mVertProgram); }

	void	bindFragProgram()		{	cgGLBindProgram(mFragProgram);	}

	void	unbindVertProgram()		{	cgGLUnbindProgram(mVertProfile); }

	void	unbindFragProgram()		{	cgGLUnbindProgram(mFragProfile); }


private:

	static   ShaderContext	_gCgContext;

	ShaderProfile	mVertProfile;			//for vertex shader

	ShaderProfile	mFragProfile;			//for fragment shader
	
	ShaderProgram	mVertProgram;

	ShaderProgram	mFragProgram;

	ShaderProgram	mShaderProgram;			// combined vertex and fragment shaders (and geometry shader?)

	ParamCgMap		mCgParamMap;			//redundant

	char			mName[MAX_NAME];


};

#endif //defined TARGET_CG_SHADERS

#endif	//CGCONTAINER_H