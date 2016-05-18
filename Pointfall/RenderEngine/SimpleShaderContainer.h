
/*
	Nnanna Kama
	Barebones class for managing Shaders
	Supports both GLSL and CG shaders (which don't have any good 3rd-party profiling tools).
	Uses a map to store parameter handles, so it's owning class don't gotta worry about such.
	ALL MATRIX GET & SET accessors presume a row-major format.
*/

#ifndef TARGET_CG_SHADERS
#define TARGET_GL_SHADERS	1
#endif

#ifndef SIMPLESHADERCONTAINER_H
#define SIMPLESHADERCONTAINER_H

#include <unordered_map>
#include <Maths\ks_Maths.h>
#include "crc32.h"

#if TARGET_CG_SHADERS
#include "..\shared_include\Cg\cg.h"
#include "..\shared_include\Cg\cgGL.h"

	typedef CGprofile		ShaderProfile;
	typedef CGcontext		ShaderContext;
	typedef CGparameter		ShaderParameter;
	typedef	CGprogram		ShaderProgram;
#else
	typedef ksU32			ShaderProfile;
	typedef ksU32			ShaderContext;
	typedef int				ShaderParameter;
	typedef ksU32			ShaderProgram;

	typedef enum ShaderAttribute
	{
		SA_POSITION		= 1,
		SA_NORMAL
	}ShaderAttribute;
#endif

#define DECLARE_PARAM(x)	inline ksU32 _getParam##x()	{ static ksU32 p = CRC32(#x); return p; }
#define PARAM_HASH(x)		_getParam##x()


	DECLARE_PARAM(globalAmbient)
	DECLARE_PARAM(lightColor)
	DECLARE_PARAM(modelViewProj)
	DECLARE_PARAM(eyePosition)
	DECLARE_PARAM(lightPosition)
	DECLARE_PARAM(Kd)
	DECLARE_PARAM(Ka)
	DECLARE_PARAM(Ke)
	DECLARE_PARAM(Ks)
	DECLARE_PARAM(shininess)

	struct ShaderKey
	{
		ShaderKey(ksU32 pKey, const char* pName) : key(pKey), name(pName)
		{}

		ksU32 key;
		const char* name;
	};

#define SHADER_KEY_DECL(x)	ShaderKey( PARAM_HASH(x), #x )


	// shader params
#define SP_GLOBAL_AMBIENT	SHADER_KEY_DECL(globalAmbient)	
#define SP_LIGHT_COL		SHADER_KEY_DECL(lightColor)		
#define SP_MODELVIEWPROJ	SHADER_KEY_DECL(modelViewProj)	
#define SP_EYE_POS			SHADER_KEY_DECL(eyePosition)
#define SP_LIGHT_POS		SHADER_KEY_DECL(lightPosition)	

#define SP_DIFFUSE			SHADER_KEY_DECL(Kd)
#define SP_AMBIENT			SHADER_KEY_DECL(Ka)
#define SP_EMISSIVE			SHADER_KEY_DECL(Ke)
#define SP_SPECULAR			SHADER_KEY_DECL(Ks)
#define SP_SHININESS		SHADER_KEY_DECL(shininess)

#define SP_POS				"in_pos"
#define SP_NOR				"in_nor"


	extern const char* myProgramName;
	extern const char* gBasicLitVertProgram;
	extern const char* gBasicLitFragProgram;
	extern const char* gLitShaderFilename;
	extern const char* gUnlitVertProgram;
	extern const char* gUnlitFragProgram;
	extern const char* gUnlitShaderFilename;



typedef std::unordered_map<ksU32, ShaderParameter >	ParamShaderMap;


class SimpleShaderContainer
{

public:
	  SimpleShaderContainer(void);

	  ~SimpleShaderContainer(void);
	
	static	ShaderContext	getStaticContext(void);

	static	void			destroyContext(void);

	static	void			destroyProgram(ShaderProgram& program);

	static	void			shaderErrorCallback(void);

	
	ShaderContext		getContext()	{ return SimpleShaderContainer::getStaticContext(); }

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
	ShaderParameter		getNamedParam( const ShaderKey& pKey );



	
	void	enableVertProfile(void);

	void	enableFragProfile(void);

	void	disableVertProfile(void);

	void	disableFragProfile(void);

	template<typename N>
	void	setFloatParameter(N param, const float value);

	template<typename N, typename V>
	void	setVectorParameter( N param, const V* value );

	template<typename N, typename V>
	void	setMatrixParameter( N param, const V* value );

	void	updateProgramParameters();

	void	getMatrixParameter(ShaderParameter, float*);

	void	bindVertProgram()	{	bindProgram(mVertProgram); }

	void	bindFragProgram()	{	bindProgram(mFragProgram);	}

	void	bindProgram()		{	bindProgram(mShaderProgram); mIsBound = true; }

	void	bindProgram(ShaderProgram& prog);

	void	unbindProgram();

	void	unbindVertProgram(void);

	void	unbindFragProgram(void);

	bool	isBound() const			{ return mIsBound; }


private:

	static   ShaderContext	_gShaderContext;

	ShaderProfile			mVertProfile;			//for vertex shader

	ShaderProfile			mFragProfile;			//for fragment shader
	
	ShaderProgram			mVertProgram;

	ShaderProgram			mFragProgram;

	ShaderProgram			mShaderProgram;

	ParamShaderMap			mShaderParamMap;

	char					mName[MAX_NAME];

	bool					mIsBound;
};

#define GL_DEBUGGING		1 && (defined _DEBUG)

#if GL_DEBUGGING
#define CHECK_GL_ERROR		SimpleShaderContainer::shaderErrorCallback()
#else
#define CHECK_GL_ERROR
#endif


#endif		//SIMPLESHADERCONTAINER_H