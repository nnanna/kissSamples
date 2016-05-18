
#include <AppLayer\GLApplication.h>
#include <Reflection\EnumReflector.h>

#define kissEnum(name, ...) enum name { __VA_ARGS__, __COUNT};

using namespace ks;

enum FStates
{
	fsHang = -2,
	fsInvalid = -1,
	fsDead,
	fsBoot,
	fsLive
};

BEGINENUMSTRING(FStates, fsInvalid)
EXPORTENUMSTRING(fsDead)
EXPORTENUMSTRING(fsBoot)
FINISHENUMSTRING(fsLive)


int main( int argc, char *argv[] )
{
	auto pstate		= fsDead;
	auto stateName	= Refl( pstate ).ToString();
	auto typeN		= Refl( pstate ).Typename();
	//typeN			= Refl( app ).Typename();
	printf("****************CAMERA CONTROLS********************\n");
	printf("W	- zoom in	S	- zoom out\n");
	printf("A	- pivot left	D	- pivot right\n");
	printf("Shift+W - lift		Shift+S - drop\n");

	GLApplication app;
	if( app.init(argc, argv) )
		app.go();

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// THE END
//////////////////////////////////////////////////////////////////////////////////////////////////////