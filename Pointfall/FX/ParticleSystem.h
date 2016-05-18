
#pragma once

#include <Maths\ks_Maths.h>
#include <Array.h>
#include <map>

struct Material;
class RenderData;

namespace ks
{
	struct ParticleController;
	struct Particles;
	struct Emitter;
}

typedef ksU32		FXID;


class ParticleSystem
{

public:
	ParticleSystem();
	~ParticleSystem();

	FXID createController(ks::ParticleController& pDesc, const char* pName);

	size_t spawn(ks::Emitter& pDesc);

	void destroy(size_t pEmitterID);

	ksU32 getUID() const								{ return mUID; }
	

	void initShader( const char* filename, const char* vp_entry, const char* fp_entry );

	void step(float elapsed);

	void setShaderParams();

	void unsetShaderParams();


private:
	void VRegister();

	ksU32					mUID;
	Material*				mMaterial;

	std::map<ks::Emitter*, ks::Particles*>	mParticleGroups;
	std::map<ks::Emitter*, RenderData*>		mRenderGroups;
	ks::Array<ks::ParticleController>		mControllers;

};


//=======================================================================================================================