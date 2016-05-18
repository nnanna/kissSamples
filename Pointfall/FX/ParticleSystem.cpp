


#include "ParticleSystem.h"
#include <FX\Particles.h>
#include <Maths\ks_Maths.inl>
#include <RenderEngine\GLRenderer.h>
#include <RenderEngine\RenderData.h>
#include <AppLayer\GLApplication.h>
#include <AppLayer\Service.h>
#include <Containers\CyclicConcurrentQueue.h>
#include <Concurrency\JobScheduler.hpp>
#include <Physics\AsyncSolver.h>
#include <Macros.h>

typedef ks::Matrix4x4	Matrix;
typedef ks::vec3		vec3;

static const ks::ParticleController	sDefaultController;

ParticleSystem::ParticleSystem() : mMaterial( nullptr )
{
	glPointSize( PARTICLE_GL_POINT_SIZE );

	initShader(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);

	VRegister();
}

//=================================================================================================================

ParticleSystem::~ParticleSystem(void)
{
	for (auto& i : mRenderGroups)
		delete i.second;

	for (auto& i : mParticleGroups)
	{
		delete i.first;
		delete i.second;
	}

	delete mMaterial;
}

FXID ParticleSystem::createController(ks::ParticleController& pDesc, const char* pName)
{
	mControllers.push_back(pDesc);
	return mControllers.size() - 1;
}


size_t ParticleSystem::spawn(ks::Emitter& pDesc)
{
	ks::Emitter* em		= new ks::Emitter(pDesc);
	ks::Particles* p	= new ks::Particles(em->mMaxParticles);
	mParticleGroups[em] = p;
	
	RenderData* rg		= new RenderData( p->positions.data(), nullptr, Matrix::IDENTITY );
	rg->vertexSize		= 3;
	rg->renderMode		= GL_POINTS;
	rg->stride			= sizeof(vec3);
	rg->material		= mMaterial;
	
	mRenderGroups[em]	= rg;

	return size_t(em);
}

void ParticleSystem::destroy(size_t pEmitterID)
{
	ks::Emitter* em = reinterpret_cast<ks::Emitter*>(pEmitterID);
	auto rgi = mRenderGroups.find(em);
	if (rgi != mRenderGroups.end())
	{
		delete rgi->second;
		mRenderGroups.erase(rgi);
	}

	auto pgi = mParticleGroups.find(em);
	if (pgi != mParticleGroups.end())
	{
		delete pgi->first;
		delete pgi->second;
		mParticleGroups.erase(pgi);
	}
}



void ParticleSystem::step(float elapsed)
{
	GLRenderer* renderer = Service<GLRenderer>::Get();

	ks::CollisionSolver::BeginBatch();

	for (auto& i : mParticleGroups)
	{
		auto& em = *i.first;
		auto& p = *i.second;
		auto& c = em.mFXID < mControllers.size() ? mControllers[ em.mFXID ] : sDefaultController;

		c.prune(p, elapsed);
		c.emit(em, p, elapsed);
		c.step(p, elapsed);

				
		//Service<ks::JobScheduler>::Get()->QueueJob( [&c, &p, elapsed]() -> ksU32
		//	{
		//		/*c.prune(p, elapsed);
		//		c.emit(em, p, elapsed);*/
		//		c.step(p, elapsed);
		//		return 0;
		//	},
		//	"FX step"
		//);
		

		if (p.live_count())
		{
			auto rg = mRenderGroups[i.first];
			rg->numIndices = p.live_count();

			renderer->addRenderData(rg);
		}
	}
	ks::CollisionSolver::EndBatch();

}

//=================================================================================================================
/*
	Registers this object with the Application and Renderer
	would'a been named register() for consistency but that's a c++ intrinsic
*/

void ParticleSystem::VRegister()
{
	mUID = Service<GLApplication>::Get()->registerObject(this);
}

//=================================================================================================================
/*
	Load a cg shader file
	Grab handles to its parameters
*/

void ParticleSystem::initShader(const char *name, const char* vp_entry, const char* fp_entry)
{
	if (mMaterial == nullptr)
	{
		mMaterial = new Material();
		mMaterial->ShaderContainer = RenderResourceFactory::findOrCreateShader(name);

		mMaterial->ShaderContainer->loadProgram(name, vp_entry, fp_entry);

		Material::setRedPlasticMaterial(mMaterial);
	}
}


//=================================================================================================================

void ParticleSystem::setShaderParams()
{}


//=================================================================================================================

void ParticleSystem::unsetShaderParams()
{
	mMaterial->ShaderContainer->disableFragProfile();
	mMaterial->ShaderContainer->disableVertProfile();
}

//=================================================================================================================

