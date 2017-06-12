
#include "SceneObject.h"
#include <defines.h>
#include <Service.h>
#include <AppLayer\GLApplication.h>
#include <Scene\Model.h>
#include <RenderEngine\RenderData.h>
#include <RenderEngine\GLRenderer.h>
#include <RenderEngine\Material.h>
#include <RenderEngine\GPUBuffer.h>

#define CHECK_INIT_MODEL	if (!mModel) mModel = new ks::Model()


SceneObject::SceneObject() : mUID(-1),
							mModel(NULL),
							mMaterial(NULL),
							mRenderData(NULL),
							mWorld(ks::Matrix::IDENTITY)
{
	VRegister();
}


//================================================================================================================


SceneObject::~SceneObject()
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

	if ( mModel->Load(filepath) )
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
	mModel->compileModel(ks::eTriangles);
	const ksU32 numIndices	= mModel->getCompiledIndexCount(ks::eTriangles);
	const ks::vec3* verts	= (const ks::vec3*)mModel->getCompiledVertices();
	ks::GPUBuffer* indices	= ks::GPUBuffer::create(numIndices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, mModel->getCompiledIndices(ks::eTriangles));
	ks::GPUBuffer* vb		= ks::GPUBuffer::create(mModel->getCompiledVertexCount(), GL_ARRAY_BUFFER, GL_STATIC_DRAW, verts);

	mRenderData				= new RenderData(vb, indices, mWorld);
	mRenderData->stride		= mModel->getCompiledVertexSize() * sizeof(float);
	mRenderData->numIndices = numIndices;
	mRenderData->normOffset = mModel->getCompiledNormalOffset() * sizeof(float);
	mRenderData->vertexSize = mModel->getPositionSize();
	mRenderData->renderMode = mModel->getPrimType();
}


//================================================================================================================


void SceneObject::initMaterial( const char* filename, const char* vp_entry, const char* fp_entry )
{
	if (mMaterial == nullptr)
	{
		mMaterial = new Material(GLApplication::loadShader(filename, vp_entry, fp_entry));

		Material::setBrassMaterial(mMaterial);
	}
}


//================================================================================================================

void SceneObject::update(float elapsed)
{
	if ( mRenderData )
	{
		mRenderData->material	= mMaterial;
		Service<ks::GLRenderer>::Get()->addRenderData( mRenderData );
	}
}

//================================================================================================================