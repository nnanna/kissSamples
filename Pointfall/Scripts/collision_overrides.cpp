
#include <Scripting\Core.h>
#include "..\Physics\CollisionDefaults.h"

struct Script : public ScriptInterface
{
	KS_SCRIPT_ON_INIT
	{
		KS_SCRIPT_ON_INIT_DEFAULT_BODY;
		DEBUG_PRINT("***collision_overrides script loaded***\n");
		
		CollisionDefaults& cd			= *(CollisionDefaults*)mDataContext;
		cd.COLLISION_RADIUS_SQ			= 1.f;
#ifndef DEBUG_VERSION
		cd.IMPULSE_FACTOR				= 0.25f;
		cd.MAX_TOTAL_COLLISIONS			= 16000;
		cd.MAX_PER_ENTITY_COLLISIONS	= 3;
#endif
	}

	KS_SCRIPT_ON_DESTROY
	{}

	KS_SCRIPT_ON_UPDATE
	{}

private:
	KS_SCRIPT_DEFAULT_MEMBERS;
};


KS_SCRIPT_EXPORT(Script)