
/*
	Nnanna Kama
	OpenGL app interface implementation
	Can be overloaded in derived class
*/

#include "Service.h"
#include <Maths/ks_Maths.inl>
#include "Macros.h"
#include "GLApplication.h"
#include "InputListener.h"
#include <RenderEngine\RenderData.h>
#include <SceneManagement\SceneObject.h>
#include <SceneManagement\Camera.h>
#include <RenderEngine\GLRenderer.h>
#include <FX\ParticleSystem.h>
#include <FX\Particles.h>
#include <Concurrency\JobScheduler.h>
#include <chrono>

typedef ks::Matrix4x4	Matrix;
typedef ks::vec4		vec4;


using namespace std::chrono;

//////////////////////////////////////////////////////////////////////////

static volatile u32 num_frames = 0;

time_point<system_clock> gStartTime, gMRUTime;
LARGE_INTEGER gFreq;
static float	gFPS(0.f);
static float	gFrameAggregate(0.f);
static int		gFrameAggregateCounter(0.f);
RenderTextDesc	gFPSData;

void initTimer()
{
	QueryPerformanceFrequency(&gFreq);
	gStartTime = gMRUTime = steady_clock::now();
}

double getElapsedTimeS()
{
	auto end				= steady_clock::now();
	duration<double> secs	= end - gMRUTime;
	gMRUTime				= end;
	
	return secs.count();
}



//================================================================================================================

GLApplication::GLApplication(void)
	: mIsFullscreen(false)
	, mRenderer(nullptr)
	, mJobScheduler(nullptr)
	, mRegisteredItemsCount(0)
	, mElapsedS(0)
{
	strcpy_s( mAppName, sizeof(mAppName), "PointFall" );
}

//================================================================================================================

GLApplication::~GLApplication(void)
{
	quit();
}


//================================================================================================================


bool GLApplication::init(int argc, char** argv)
{
	if( mRenderer )
		return true;

	glutInit(&argc, argv);
	glutInitWindowSize(600, 400);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

	glutCreateWindow(mAppName);
	glutDisplayFunc(GLRenderer::render_callback);
	glutReshapeFunc(GLRenderer::reshape_callback);
	glutKeyboardFunc(InputListener::KeyDownCallback);
	glutKeyboardUpFunc(InputListener::KeyUpCallback);
	glutTimerFunc( 1, GLApplication::update_callback, 1 );

	glClearColor(0.5f, 0.5f, 0.5f, 0.f);  // Gray background.
	glEnable(GL_DEPTH_TEST);

	Service<GLApplication>::Register( this );

	mRenderer = new GLRenderer();
	Service<GLRenderer>::Register( mRenderer );

	mJobScheduler = new ks::JobScheduler(3, 7);		// 3 workers, 7 jobs max, multi-producer
	Service<ks::JobScheduler>::Register( mJobScheduler );


	//glutCreateMenu(menu);
	//glutAddMenuEntry("[ ] Animate", ' ');
	//glutAttachMenu(GLUT_RIGHT_BUTTON);

	SceneObject* floor = new SceneObject();
	{
		floor->initMaterial(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);
		floor->loadQuad(1.f);
		Matrix rot(Matrix::IDENTITY), scale(Matrix::IDENTITY), trans;
		rot.SetRotateX(DEG_TO_RAD(90.f));
		float scaling = 401.f;
		scale.SetScaling(scaling);
		scale.m24 = scaling - 0.1f;		// offset the y-axis slightly below 0

		trans = scale * rot;
		floor->setMatrix(trans);
	}

	/*SceneObject* cube = new SceneObject();
	{
		cube->initMaterial(gLitShaderFilename, gBasicLitVertProgram, gBasicLitFragProgram);
		cube->loadCube(1.5f);
		cube->getMatrix().m24 += 1.4f;
	}*/

	ParticleSystem* pSys = new ParticleSystem();
	{
		pSys->initShader(gLitShaderFilename, gBasicLitVertProgram, gBasicLitFragProgram);

		ks::ParticleController c;
		c.SizeDurationRange		= vec4( 0.2f, 0.2f, 10.4f, 11.8f );
		c.InitVelocityRange		= vec3(0.7f, 0.5f, 0.9f);
		FXID fxid				= pSys->createController(c, "PointFall");

		ks::Emitter emz;
		emz.mWorldPos			= vec3(-17.393f, 12.405f, 0.10106f);	// initialise to weird values, like a pro
		emz.mEmissionVelocity	= vec3( 11.3778f, 4.6492f, 0.11203f);
		emz.mMaxParticles		= 3069;									// lower max to stagger emission
		emz.mEmissionRate		= 417;
		emz.mFXID				= fxid;

		pSys->spawn( emz );


		emz.mWorldPos			= vec3(3.f, 15.405f, 0.f);
		emz.mEmissionVelocity	= vec3(-3.f, -0.6492f, 0.011203f);
		emz.mMaxParticles		= 4173;
		emz.mEmissionRate		= 300;

		pSys->spawn( emz );
	}

	//sceneObj->loadModel("..\\media\\models\\venusm.obj");
	//sceneObj->loadModel("..\\media\\models\\crysponza_bubbles\\sponza.obj");

	initTimer();

	//printf("frame time = xxxxxxx");

	return true;
}


//================================================================================================================

void GLApplication::destroy(ks32 exit_value_or_whatever_this_crap_is)
{
	Service<GLApplication>::Get()->quit();
	exit(0);
}


//================================================================================================================

template<>
ks::Array<SceneObject*>& GLApplication::getObjectCollection()
{
	return mSceneObjects;
}

template<>
ks::Array<ParticleSystem*>& GLApplication::getObjectCollection()
{
	return mParticleSubsytems;
}



void GLApplication::update(ks32 pCallbackID)
{
	mElapsedS = getElapsedTimeS();
	if (mElapsedS > 0.05f)
		mElapsedS = 0.05f;	// clamp for when glut suspends app update on window resize or drag. sigh.



#define FRAME_AGGREGATE_LIMIT	10
	if (gFrameAggregateCounter++ == FRAME_AGGREGATE_LIMIT)
	{
		gFPS = float(FRAME_AGGREGATE_LIMIT) / gFrameAggregate;
		gFrameAggregate = 0.f;
		gFrameAggregateCounter = 0;
	}
	gFrameAggregate += mElapsedS;

	sprintf_s(gFPSData.text, "FPS : %.1f", gFPS);
	mRenderer->addRenderTextData(gFPSData);
	
	
	for (size_t i = 0; i < mParticleSubsytems.size(); ++i)
	{
		mParticleSubsytems[i]->step(mElapsedS);
	}

	for (size_t i = 0; i < mSceneObjects.size(); ++i)
	{
		mSceneObjects[i]->update(mElapsedS);
	}

	CameraManager::update(mElapsedS, false);

	mRenderer->update(mElapsedS);

	glutPostRedisplay();
}


//================================================================================================================



void GLApplication::go()
{	
	glutMainLoop();			// bye-bye, control

	/*mHWND = hwnd;

	if(!init(mHWND))
		return;

	while(isRunning)
	{
		update();
		render();
	}*/
}


//================================================================================================================


void GLApplication::quit()
{
	delete mJobScheduler;

	for(size_t i = 0; i < mParticleSubsytems.size(); i++)
	{
		delete mParticleSubsytems[i];
	}
	mParticleSubsytems.clear();
	
	for (size_t i = 0; i < mSceneObjects.size(); i++)
	{
		delete mSceneObjects[i];
	}
	mSceneObjects.clear();

	CameraManager::destroy();

	delete mRenderer;
}


//================================================================================================================

void GLApplication::update_callback(ks32 pCallbackID)
{
	Service<GLApplication>::Get()->update(pCallbackID);
	glutTimerFunc(1, GLApplication::update_callback, pCallbackID);
}


//================================================================================================================
