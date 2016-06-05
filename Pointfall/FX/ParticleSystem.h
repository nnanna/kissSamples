
#pragma once

#include <defines.h>
#include <Array.h>
#include <map>


namespace ks
{
	struct ParticleController;
	struct Particles;
	struct Emitter;
	struct Material;
	class RenderData;
	class JobGroup;
}

typedef ksU32			FXID;
typedef ks::RenderData	RenderData;
typedef ks::Material	Material;


class ParticleSystem
{

public:
	ParticleSystem();
	~ParticleSystem();

	FXID createController(ks::ParticleController& pDesc, const char* pName);

	size_t spawn(ks::Emitter& pDesc);

	void destroy(size_t pEmitterID);

	ksU32 getUID() const								{ return mUID; }
	

	void initMaterial( const char* shadername, const char* vp_entry, const char* fp_entry );

	void step(float elapsed);


private:
	void VRegister();

	ksU32					mUID;
	Material*				mMaterial;
	ks::JobGroup*			mJobStream;

	std::map<ks::Emitter*, ks::Particles*>	mParticleGroups;
	std::map<ks::Emitter*, RenderData*>		mRenderGroups;
	ks::Array<ks::ParticleController>		mControllers;

};


//=======================================================================================================================