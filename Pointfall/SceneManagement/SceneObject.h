#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "defines.h"
#include <Maths/ks_Maths.h>


namespace ks
{
	class Model;
	class RenderData;
	struct Material;
}

typedef ks::Model		Model;
typedef ks::Matrix		Matrix;
typedef ks::RenderData	RenderData;
typedef ks::Material	Material;



class SceneObject
{
public:
	SceneObject();

	~SceneObject();

	void update(float elapsed);

	void loadModel(const char* filepath);
	void loadCube( float pSize );
	void loadQuad(float pSize);

	void initMaterial( const char* shader_filename);


	ks::Model*		getModel()				{return mModel;}

	ks32			getUID() const			{ return mUID; }

	const Matrix& getMatrix() const			{ return mWorld; }

	Matrix& getMatrix()						{ return mWorld; }

	void setMatrix(const Matrix& pVal)		{ mWorld = pVal; }


private:

	void VRegister();

	void				fillRenderData();

	ks::Model*			mModel;

	Material*			mMaterial;

	RenderData*			mRenderData;

	int					mUID;					// this should ideally be constant, right? err, whatever.

	Matrix				mWorld;
};


#endif