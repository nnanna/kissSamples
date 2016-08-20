
#include <Scripting\Core.h>
#include <Maths\ks_Maths.inl>
#include "..\FX\Particles.h"

struct Script : public ScriptInterface
{
	KS_SCRIPT_ON_INIT
	{
		KS_SCRIPT_ON_INIT_DEFAULT_BODY;
		DEBUG_PRINT("***hero_emitter_overrides script loaded***\n");
		
		ks::Emitter & emz		= *(ks::Emitter*)mDataContext;
#ifndef DEBUG_VERSION
		emz.mEmissionRate		= 700;
#endif
		emz.mWorldPos			= vec3(-13.393f, 5.405f, 0.10106f);
		emz.mEmissionVelocity	= vec3(18.3778f, 9.6492f, 0.11203f);
	}

	KS_SCRIPT_ON_DESTROY
	{}

	KS_SCRIPT_ON_UPDATE
	{}

private:
	KS_SCRIPT_DEFAULT_MEMBERS;
};


KS_SCRIPT_EXPORT(Script)