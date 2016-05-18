
#include "SceneObject.h"
#include <Macros.h>
#include <AppLayer\Service.h>
#include <AppLayer\GLApplication.h>
#include <AssetLoader\kissModel.h>
#include <RenderEngine\RenderData.h>
#include <RenderEngine\GLRenderer.h>

#define CHECK_INIT_MODEL	if (!mModel) mModel = new Model()


using namespace ks;

SceneObject::SceneObject(void) : mUID(-1),
								mModel(NULL),
								mMaterial(NULL),
								mRenderData(NULL),
								mWorld(Matrix4x4::IDENTITY)
{
	VRegister();
}


//================================================================================================================


SceneObject::~SceneObject(void)
{
	delete mRenderData;
	delete mMaterial;
	delete mModel;
}


//================================================================================================================


void SceneObject::VRegister()
{
	mUID = Service<GLApplication>::Get()->registerObject( this );
}

//================================================================================================================


void SceneObject::loadCube( float pSize )
{
	CHECK_INIT_MODEL;
	mModel->makeCube(pSize);
	fillRenderData();
}

//================================================================================================================


void SceneObject::loadQuad(float pSize)
{
	CHECK_INIT_MODEL;
	mModel->makeQuad(pSize);
	fillRenderData();
}

//================================================================================================================


void SceneObject::loadModel(const char* filepath)
{
	CHECK_INIT_MODEL;

	if ( mModel->loadModelFromFile(filepath) )
	{
		fillRenderData();
	}
	else
	{
		printf("Error loading model '%s'\n", filepath);
		SAFE_DELETE(mModel);
	}
}

//================================================================================================================

void SceneObject::fillRenderData()
{
	if (mRenderData)
		delete mRenderData;

	mModel->computeNormals();
	mModel->compileModel(eptAll);
	auto verts				= mModel->getCompiledVertices();
	auto indies				= mModel->getCompiledIndices(eptTriangles);
	mRenderData				= new RenderData(verts, indies, mWorld);
	mRenderData->stride		= mModel->getCompiledVertexSize() * sizeof(float);
	mRenderData->numIndices = mModel->getCompiledIndexCount(eptTriangles);
	mRenderData->normOffset = mModel->getCompiledNormalOffset();
	mRenderData->vertexSize = mModel->getPositionSize();
	mRenderData->renderMode = mModel->getPrimType() == eptNone ? GL_QUADS : GL_TRIANGLES;
}


//================================================================================================================


void SceneObject::initMaterial( const char* filename, const char* vp_entry, const char* fp_entry )
{
	mMaterial = new Material( RenderResourceFactory::findOrCreateShader( filename ) );

	Material::setBrassMaterial( mMaterial );

	mMaterial->ShaderContainer->loadProgram(filename, vp_entry, fp_entry);
}


//================================================================================================================

void SceneObject::update(float elapsed)
{
	if ( mRenderData )
	{
		mRenderData->material	= mMaterial;
		Service<GLRenderer>::Get()->addRenderData( mRenderData );
	}
}

//================================================================================================================