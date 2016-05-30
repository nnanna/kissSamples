


#include "ParticleSystem.h"
#include <Service.h>
#include <defines.h>
#include <FX\Particles.h>
#include <Maths\ks_Maths.h>
#include <Physics\AsyncSolver.h>
#include <RenderEngine\Material.h>
#include <RenderEngine\GLRenderer.h>
#include <RenderEngine\RenderData.h>
#include <AppLayer\GLApplication.h>
#include <Concurrency\JobScheduler.hpp>
#include <Memory\ThreadStackAllocator.h>

typedef ks::Matrix	Matrix;
typedef ks::vec3	vec3;

static const ks::ParticleController	sDefaultController;

ParticleSystem::ParticleSystem() : mMaterial( nullptr )
{
	VRegister();
}

//=================================================================================================================

ParticleSystem::~ParticleSystem()
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
	rg->renderMode		= ks::ePoints;
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


#define CONCURRENT_FX_UPDATE	1

void ParticleSystem::step(float elapsed)
{
#if CONCURRENT_FX_UPDATE
	ks::JobScheduler* scheduler = Service<ks::JobScheduler>::Get();

	ks::mem::ThreadStackAllocator stack(mParticleGroups.size() * sizeof(ks::JobHandle));
	ks::JobHandle* particleJobs = (ks::JobHandle*)stack.allocate();
	ksU32 jobIndex(0);
#endif

	ks::CollisionSolver::BeginBatch();

	for (auto& i : mParticleGroups)
	{
		auto& em = *i.first;
		auto& p = *i.second;
		auto& c = em.mFXID < mControllers.size() ? mControllers[ em.mFXID ] : sDefaultController;

#if CONCURRENT_FX_UPDATE
		particleJobs[ jobIndex ] = scheduler->QueueJob(
			[&c, &p, &em, elapsed, jobIndex]() -> int
			{
				c.prune(p, elapsed);
				c.emit(em, p, elapsed);
				c.step(p, elapsed);
				return jobIndex;
			},

			[&i, this](int pJobIndex)
			{
				auto& p = *i.second;
				if (p.live_count())
				{
					auto rg = mRenderGroups[i.first];
					rg->numIndices = p.live_count();

					Service<ks::GLRenderer>::Get()->addRenderData(rg);
				}
			},
			"FX step"
		);
		++jobIndex;

#else
		c.prune(p, elapsed);
		c.emit(em, p, elapsed);
		c.step(p, elapsed);
		if (p.live_count())
		{
			auto rg = mRenderGroups[i.first];
			rg->numIndices = p.live_count();

			Service<ks::GLRenderer>::Get()->addRenderData(rg);
		}
#endif
	}

#if CONCURRENT_FX_UPDATE
	for (ksU32 j = 0; j < jobIndex; ++j)
		particleJobs[j].Sync();
#endif

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

void ParticleSystem::initMaterial(const char *name, const char* vp_entry, const char* fp_entry)
{
	if (mMaterial == nullptr)
	{
		mMaterial = new Material( GLApplication::loadShader(name, vp_entry, fp_entry) );
		Material::setRedPlasticMaterial(mMaterial);
	}
}

