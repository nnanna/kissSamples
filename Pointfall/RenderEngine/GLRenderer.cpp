/*
	Nnanna Kama
	Render class Impl
*/



#include "Macros.h"
#include "GLRenderer.h"
#include "Light.h"
#include "RenderData.h"
#include "RenderResourceFactory.h"
#include <FX\ParticleSystem.h>
#include <SceneManagement\SceneObject.h>
#include <SceneManagement\Camera.h>
#include <AppLayer\GLApplication.h>
#include <AppLayer\Service.h>

typedef ks::Matrix4x4	Matrix;


const float gLightRadius		= 25.f;
const Light gDefaultLight		= { vec3(gLightRadius*sin(myLightAngle), 50.5f, gLightRadius*cos(myLightAngle) ), vec3( 0.95f, 0.95f, 0.95f ) };
const vec3 gGlobalAmbient( 0.1f, 0.1f, 0.1f );

static Material gFontMaterial;

void GLRenderer::drawTextData()
{
	glRasterPos3f(-1, 0.92, 0);
	gFontMaterial.SetShaderParams();

	for (auto& td : mRenderTextBuffer)
	{
		auto& buffer = td.text;
		for (int i = 0; i < RenderTextDesc::MAX_SIZE && buffer[i] != '\0'; ++i)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
	}
}


///////////////////////////////////////////////////////////////////////////////////
template<>				ksU32 getGLType(const float* pT)		{ return GL_FLOAT; }
template<>				ksU32 getGLType(const ksU32* pT)		{ return GL_UNSIGNED_INT; }
template<>				ksU32 getGLType(const ksUShort* pT)		{ return GL_UNSIGNED_SHORT; }
template<>				ksU32 getGLType(const ksByte* pT)		{ return GL_UNSIGNED_BYTE; }
///////////////////////////////////////////////////////////////////////////////////

//================================================================================================================

GLRenderer::GLRenderer(void) : mDefaultMaterial(NULL), mMRUShader(NULL)
{
	mDefaultMaterial = new Material(RenderResourceFactory::findOrCreateShader(gUnlitShaderFilename));
	Material::setRedPlasticMaterial(mDefaultMaterial);

	mDefaultMaterial->ShaderContainer->loadProgram(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);
	mDefaultMaterial->ShaderContainer->bindProgram();
	mDefaultMaterial->ShaderContainer->setVectorParameter(SP_GLOBAL_AMBIENT, &gGlobalAmbient);
	mDefaultMaterial->ShaderContainer->setVectorParameter(SP_LIGHT_COL, &gDefaultLight.color);
	mDefaultMaterial->ShaderContainer->unbindProgram();

	gFontMaterial.SetDiffuse(0, 0, 0);
	gFontMaterial.ShaderContainer = RenderResourceFactory::findOrCreateShader(gUnlitShaderFilename);
	//gFontMaterial.ShaderContainer->loadProgram(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);

	CameraManager::createCamera();
}


//================================================================================================================


GLRenderer::~GLRenderer(void)
{
	SAFE_DELETE(mDefaultMaterial);
	mRenderData.clear();
	RenderResourceFactory::shutDown();
}



//================================================================================================================

void GLRenderer::update( float pElaspedS )
{
	CHECK_GL_ERROR;
}


//================================================================================================================

void GLRenderer::addRenderData( const RenderData* r )
{
	mRenderData.push_back( r );
}

void GLRenderer::addRenderTextData(const RenderTextDesc& pDesc)
{
	mRenderTextBuffer.push_back(pDesc);
}

//================================================================================================================
/*
	This should be auto-called every frame
	to prevent the buffers from growing infinitely
	TODO: clearing and repopulating the render-list every frame is potentially expensive
	Only remove items when necessary.
*/

void GLRenderer::flushRenderData()
{
	mRenderData.clear();
	mRenderTextBuffer.clear();
}


void GLRenderer::uploadShaderConstants( Material* pMat, const Camera* pCam, const Matrix& pTransform )
{
	SimpleShaderContainer* shader	= pMat->ShaderContainer;
	const vec3& camPos				= pCam->getPosition();
	const Matrix& view				= pCam->getView();
	const Matrix& projection		= pCam->getProjection();
	
	Matrix inv( pTransform.Inverse() );
	Matrix mvp( projection * view * pTransform );

	vec3 invEye( inv.TransformNormal( camPos ) );
	vec3 invLight( inv.TransformNormal( gDefaultLight.position ) );


	if( mMRUShader != shader )
	{
		if ( mMRUShader )
		{
			mMRUShader->disableVertProfile();
			mMRUShader->disableFragProfile();
			mMRUShader->unbindProgram();
		}

		if ( !shader->isBound() )
		{
			shader->bindProgram();
			shader->enableVertProfile();
			shader->enableFragProfile();
		}
	}
	
	
	shader->setVectorParameter( SP_EYE_POS,			&invEye);
	shader->setVectorParameter( SP_LIGHT_POS,		&invLight);
	shader->setMatrixParameter( SP_MODELVIEWPROJ,	&mvp);		// Set parameter with row-major matrix.

	pMat->SetShaderParams();
}


//================================================================================================================

void GLRenderer::render()
{
	CHECK_GL_ERROR;

	const Camera* cam		= CameraManager::getMainCamera();

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );

	for( const RenderData* rd : mRenderData )
	{
		Material *mat		= rd->material ? rd->material : mDefaultMaterial;

		uploadShaderConstants( mat, cam, rd->Transform );

		const float* vb		= (const float*)rd->vertexBuffer;
		const ksU32* ib		= rd->indexBuffer;
		int vert_size		= rd->vertexSize;
		int stride			= rd->stride;
		int num_indices		= rd->numIndices;
		const float* norms	= vb + rd->normOffset;

		glVertexPointer( vert_size, GET_GLTYPE(vb), stride, vb);
		if (rd->normOffset)
			glNormalPointer( GET_GLTYPE(norms), stride, norms);

#ifdef TARGET_GL_SHADERS
		// these correspond to glBindAttribLocation()
		glVertexAttribPointer(SA_POSITION,	vert_size, GET_GLTYPE(vb),		GL_FALSE, stride, vb);
		if ( rd->normOffset )
			glVertexAttribPointer(SA_NORMAL,vert_size, GET_GLTYPE(norms),	GL_FALSE, stride, norms);
#endif
		if (ib)
			glDrawElements(rd->renderMode, num_indices, GET_GLTYPE(ib), ib);
		else
			glDrawArrays(rd->renderMode, 0, num_indices);

		mMRUShader = mat->ShaderContainer;
	}

	drawTextData();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );

	glutSwapBuffers();

	flushRenderData();

	CHECK_GL_ERROR;
	
	//glutReportErrors();
}


//================================================================================================================


void GLRenderer::reshape(int width, int height) const
{
	float aspect	= float(width) / float(height);
	for( ksU32 i = 0; i < CameraManager::getNumCameras(); ++i )
	{
		Camera* cam	= CameraManager::getCamera(i);

		cam->setAspectRatio( aspect );
		cam->refreshProjectionMatrix();
	}
	glViewport(0, 0, width, height);
}


//================================================================================================================

void GLRenderer::render_callback()
{
	Service<GLRenderer>::Get()->render();
}


//================================================================================================================

void GLRenderer::reshape_callback(int width, int height)
{
	Service<GLRenderer>::Get()->reshape(width, height);
}


//================================================================================================================