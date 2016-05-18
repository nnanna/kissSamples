#ifndef GL_RENDENDER
#define GL_RENDENDER

/*
	Nnanna Kama
	Render class
*/

#include <Array.h>
#include "RenderResourceFactory.h"
#include "Material.h"

#if TARGET_CG_SHADERS
#include "glut.h"
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif



class Camera;
class RenderData;
struct RenderTextDesc;
class ParticleSystem;
class SimpleShaderContainer;

namespace ks
{
	class Matrix4x4;
}

typedef ks::Array<const RenderData*>	RenderDataArray;

class GLRenderer
{
public:

	GLRenderer();

	virtual ~GLRenderer();

	static  void	render_callback(void);

	static	void	reshape_callback(int width, int height);

	void	render();

	void	reshape(int width, int height) const;

	void	update(float pElapsedS);

	void	addRenderData( const RenderData* r );

	void	addRenderTextData(const RenderTextDesc& pDesc);

	void	flushRenderData( void );


private:

	void	uploadShaderConstants( Material* pMat, const Camera* pCam, const ks::Matrix4x4& pTransform );

	void	drawTextData();

	Material*		mDefaultMaterial;

	SimpleShaderContainer* mMRUShader;					// most recently used shader.

	
	/*
		data: array of vertex & index buffers for rendering
	*/
	RenderDataArray				mRenderData;

	ks::Array<RenderTextDesc>	mRenderTextBuffer;

};


static float myLightAngle = -0.4f;   /* Angle light rotates around scene. */


template<typename T>	ksU32 getGLType(const T* pT);

#define GET_GLTYPE(x)	getGLType(x)

#endif