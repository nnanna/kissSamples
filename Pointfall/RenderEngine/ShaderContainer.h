
/*
	Nnanna Kama
	Interface/Abstract Shader class. Inherited by CgContainer and GLSLContainer.
*/

#ifndef SHADERCONTAINER_H
#define SHADERCONTAINER_H

class IShaderContainer
{

public:

	virtual ~IShaderContainer() {};

	// these parameters differ depending on if we're using Cg or GLSL.
	// it's Cg based here. But for GLSL, filename is left NULL
	// vertEntry == VertexShader filename
	// fragEntry == fragmentShader filename
	virtual ShaderProgram		loadProgram(const char* filename, const char* vertEntry, const char* fragEntry) = 0;

	virtual ShaderProgram		loadProgram(const char *filename, const char *entry, ShaderProfile profile) = 0;

	virtual ShaderContext		getContext() = 0;

	
// get named vertex and fragment program parameters

	virtual ShaderParameter		getNamedParam( const char* name ) = 0;



	
	virtual void	enableVertProfile(void) = 0;

	virtual void	enableFragProfile(void) = 0;

	virtual void	disableVertProfile(void) = 0;

	virtual void	disableFragProfile(void) = 0;

	virtual void	setFloatParameter(ShaderParameter param, const float value) = 0;

	virtual void	setFloatParameter(const char* name,	const float value) = 0;

	virtual void	setVectorParameter(ShaderParameter param, const VECTOR3& value) = 0;

	virtual void	setVectorParameter(ShaderParameter param, const float* value) = 0;

	virtual void	setVectorParameter(const char* name, const VECTOR3& value) = 0;

	virtual void	setVectorParameter(const char* name, const float* value) = 0;

	virtual void	setMatrixParameter(const char* name, const Matrix4x4* value) = 0;

	virtual void	setMatrixParameter(const char* name, float* value) = 0;

	virtual void	setMatrixParameter(ShaderParameter param, const Matrix4x4* value) = 0;

	virtual void	setMatrixParameter(ShaderParameter param, float* value) = 0;

	virtual void	setStateMatrixParameter(ShaderParameter) = 0;

	virtual void	updateProgramParameters() = 0;

	virtual void	getMatrixParameter(ShaderParameter, float*) = 0;

	virtual	void	bindProgram() = 0;

	virtual void	unbindProgram() = 0;

	virtual void	bindVertProgram() = 0;

	virtual void	bindFragProgram() = 0;

	virtual void	unbindVertProgram() = 0;

	virtual void	unbindFragProgram() = 0;

	virtual bool	isBound() const		= 0;

};


#endif