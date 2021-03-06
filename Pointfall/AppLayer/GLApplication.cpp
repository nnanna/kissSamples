
/*
	Nnanna Kama
	OpenGL app interface implementation
	Can be overloaded in derived class
*/

#include <RenderEngine\RenderResourceFactory.h>
#include "Service.h"
#include <Maths/ks_Maths.inl>
#include "defines.h"
#include "GLApplication.h"
#include "InputListener.h"
#include <RenderEngine\RenderData.h>
#include <SceneManagement\SceneObject.h>
#include <SceneManagement\Camera.h>
#include <RenderEngine\Material.h>
#include <RenderEngine\GLRenderer.h>
#include <RenderEngine\GL\glew.h>
#include <RenderEngine\GL\wglew.h>
#include <RenderEngine\GL\glut.h>
#include <FX\ParticleSystem.h>
#include <FX\Particles.h>
#include <Concurrency\JobScheduler.h>
#include <chrono>
#include <Scripting\ScriptFactory.h>
#include <Scripting\ScriptInterface.h>
#include <Profiling\Trace.h>


#define HERO_EMIITER_SCRIPT		"hero_emitter_overrides"

typedef ks::Matrix	Matrix;
typedef ks::vec4	vec4;

using namespace std::chrono;

ks::ScriptFactory sf(nullptr);
ks::ScriptInterface* sScript = nullptr;
uintptr_t gHeroFXContext = 4.f;

struct RenderTextDesc
{
	static const u32 MAX_SIZE = 32;
	char		text[MAX_SIZE];
	Material*	mat;
	struct
	{
		float x, y;
	}normalised_scr_coords;
};

//////////////////////////////////////////////////////////////////////////

static volatile u32 num_frames = 0;
static u32 INTERFRAME_UPDATE_INTERVAL = 0;

void SetVSync(int sync)
{
	if (sync)
		INTERFRAME_UPDATE_INTERVAL = sync;
	else
	{
		// ain't nobody got time to check WGL_EXT_swap_control
		BOOL success = wglSwapIntervalEXT(sync);
		if (success && sync == 0)
			INTERFRAME_UPDATE_INTERVAL = 12;			// target 60-70fps clamp so the simulation isn't overly dampened
	}
}

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


DECLARE_SHADER_CONSTANT(globalAmbient)
DECLARE_SHADER_CONSTANT(lightColor)
DECLARE_SHADER_CONSTANT(modelViewProj)
DECLARE_SHADER_CONSTANT(eyePosition)
DECLARE_SHADER_CONSTANT(lightPosition)
DECLARE_SHADER_CONSTANT(Kd)
DECLARE_SHADER_CONSTANT(Ka)
DECLARE_SHADER_CONSTANT(Ke)
DECLARE_SHADER_CONSTANT(Ks)
DECLARE_SHADER_CONSTANT(shininess)


#define SC_GLOBAL_AMBIENT	SHADER_KEY_DECL(globalAmbient)	
#define SC_LIGHT_COL		SHADER_KEY_DECL(lightColor)		
#define SC_MODELVIEWPROJ	SHADER_KEY_DECL(modelViewProj)	
#define SC_EYE_POS			SHADER_KEY_DECL(eyePosition)
#define SC_LIGHT_POS		SHADER_KEY_DECL(lightPosition)	

#define SC_DIFFUSE			SHADER_KEY_DECL(Kd)
#define SC_AMBIENT			SHADER_KEY_DECL(Ka)
#define SC_EMISSIVE			SHADER_KEY_DECL(Ke)
#define SC_SPECULAR			SHADER_KEY_DECL(Ks)
#define SC_SHININESS		SHADER_KEY_DECL(shininess)

static Material gFontMaterial;

static const int TOTAL_SHADER_CONSTANTS = 10;
static ks::ShaderKey gShaderConstants[TOTAL_SHADER_CONSTANTS] =	// ordering needs to be same as Material::ShaderConstantsID
{
	SC_DIFFUSE,
	SC_AMBIENT,
	SC_EMISSIVE,
	SC_SPECULAR,
	SC_SHININESS,

	SC_GLOBAL_AMBIENT,
	SC_LIGHT_COL,
	SC_EYE_POS,
	SC_LIGHT_POS,
	SC_MODELVIEWPROJ,
};


const char	*gLitShaderFilename		= "media\\programs\\basicLit.glsl",
			*gUnlitShaderFilename	= "media\\programs\\basicUnlit.glsl";

//================================================================================================================

GLApplication::GLApplication()
	: mIsFullscreen(false)
	, mRenderer(nullptr)
	, mJobScheduler(nullptr)
	, mRegisteredItemsCount(0)
	, mElapsedS(0)
{
	strcpy_s( mAppName, sizeof(mAppName), "PointFall" );
	gFPSData.mat	= &gFontMaterial;
	gFPSData.normalised_scr_coords.x = -1.f;
	gFPSData.normalised_scr_coords.y = 0.92f;
}

//================================================================================================================

GLApplication::~GLApplication()
{
	quit();
}


//================================================================================================================


bool GLApplication::init(int argc, char** argv)
{
	if( mRenderer )
		return true;

	ks::TraceAPI::Init(512);

	glutInit(&argc, argv);
	glutInitWindowSize(600, 400);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutCreateWindow(mAppName);

	glClearColor(0.5f, 0.5f, 0.5f, 0.f);  // Gray background.
	glEnable(GL_DEPTH_TEST);

	Service<GLApplication>::Register( this );

	mRenderer = new ks::GLRenderer();
	Service<ks::GLRenderer>::Register(mRenderer);

	CameraManager::createCamera();

	mJobScheduler = new ks::JobScheduler(3, 32);		// 3 workers, 32 jobs max, multi-producer
	Service<ks::JobScheduler>::Register( mJobScheduler );

	gFontMaterial.SetDiffuse(0, 0, 0);
	gFontMaterial.ShaderContainer = loadShader(gUnlitShaderFilename);

	//glutCreateMenu(menu);
	//glutAddMenuEntry("[ ] Animate", ' ');
	//glutAttachMenu(GLUT_RIGHT_BUTTON);

	SceneObject* floor = new SceneObject();
	{
		floor->initMaterial(gUnlitShaderFilename);
		floor->loadQuad(1.f);
		Matrix rot(Matrix::IDENTITY), scale(Matrix::IDENTITY), trans;
		rot.SetRotateX(DEG_TO_RAD(90.f));
		float scaling = 401.f;
		scale.SetScaling(scaling);
		scale.m24 = scaling - 0.1f;		// offset the y-axis slightly below 0

		trans = scale * rot;
		floor->setMatrix(trans);
	}

	SceneObject* venus = new SceneObject();
	{
		venus->initMaterial(gLitShaderFilename);
		venus->loadModel("media//models//venusm.obj");
		Matrix trans(Matrix::IDENTITY);
		float scaling = 0.001f;
		trans.SetScaling(scaling);
		trans.m24 = scaling + 0.7f;		// offset the y-axis slightly above 0

		venus->setMatrix(trans);
	}

	/*SceneObject* cube = new SceneObject();
	{
		cube->initMaterial(gLitShaderFilename, gBasicLitVertProgram, gBasicLitFragProgram);
		cube->loadCube(1.5f);
		cube->getMatrix().m24 += 1.4f;
	}*/

	ParticleSystem* pSys = new ParticleSystem();
	{
		pSys->initMaterial(gUnlitShaderFilename);

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
		gHeroFXContext = pSys->spawn( emz );


		emz.mWorldPos			= vec3(3.f, 15.405f, 0.f);
		emz.mEmissionVelocity	= vec3(-3.f, -0.6492f, 0.011203f);
		emz.mMaxParticles		= 4173;
		emz.mEmissionRate		= 300;
		pSys->spawn( emz );


		emz.mWorldPos			= vec3(-3.f, 18.115f, 0.f);
		emz.mEmissionVelocity	= vec3(0.f, -0.2492f, 0.f);
		emz.mMaxParticles		= 3173;
		emz.mEmissionRate		= 200;
		pSys->spawn( emz );

		emz.mWorldPos			= vec3(7.f, 12.115f, 0.f);
		emz.mEmissionVelocity	= vec3(-10.f, 2.1f, 0.f);
		emz.mMaxParticles		= 1022;
		emz.mEmissionRate		= 99;
		pSys->spawn( emz );
	}
	glPointSize( PARTICLE_GL_POINT_SIZE );

	//sceneObj->loadModel("..\\media\\models\\venusm.obj");
	//sceneObj->loadModel("..\\media\\models\\crysponza_bubbles\\sponza.obj");

	initTimer();

	Material::gConstantsRegistry = gShaderConstants;

	SetVSync(0);
	glutDisplayFunc(GLApplication::render_callback);
	glutReshapeFunc(GLApplication::reshape_callback);
	glutKeyboardFunc(InputListener::KeyDownCallback);
	glutKeyboardUpFunc(InputListener::KeyUpCallback);
	glutTimerFunc(0, GLApplication::update_callback, 1);

	if (sScript == nullptr)
	{
		sScript = sf.Load(HERO_EMIITER_SCRIPT, (void*)gHeroFXContext);
	}

	ks::TraceAPI::Start();
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



void GLApplication::update(ks32 pFrameID)
{
	TRACE_FUNC();

	mElapsedS = getElapsedTimeS();
	if (mElapsedS > 0.05f)
		mElapsedS = 0.05f;	// clamp for when glut suspends app update on window resize or drag. sigh.

	const ksU32 upKey = InputListener::getKeyUp();
	if (upKey == 'r')
	{
		sScript = sf.Load(HERO_EMIITER_SCRIPT, (void*)gHeroFXContext, true);
	}
	if (sScript) sScript->Update(mElapsedS);

	for (size_t i = 0; i < mParticleSubsytems.size(); ++i)
	{
		mParticleSubsytems[i]->step(mElapsedS);
	}

#define FRAME_AGGREGATE_LIMIT	10
	if (gFrameAggregateCounter++ == FRAME_AGGREGATE_LIMIT)
	{
		gFPS = float(FRAME_AGGREGATE_LIMIT) / gFrameAggregate;
		gFrameAggregate = 0.f;
		gFrameAggregateCounter = 0;
	}
	gFrameAggregate += mElapsedS;

	sprintf_s(gFPSData.text, "FPS : %.1f", gFPS);
	addRenderTextData(gFPSData);
	

	for (size_t i = 0; i < mSceneObjects.size(); ++i)
	{
		mSceneObjects[i]->update(mElapsedS);
	}

	CameraManager::update(mElapsedS, false);

	mRenderer->update(mElapsedS);

	InputListener::onFrameEnd();

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
	sf.Unload(sScript);
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

	delete mJobScheduler;

	delete mRenderer;

	ks::TraceAPI::Destroy();
}


//================================================================================================================

void GLApplication::update_callback(ks32 pFrameNumber)
{
	Service<GLApplication>::Get()->update(pFrameNumber);
	glutTimerFunc(INTERFRAME_UPDATE_INTERVAL, GLApplication::update_callback, ++pFrameNumber);
}

//================================================================================================================

void GLApplication::reshape_callback(int width, int height)
{
	float aspect = float(width) / float(height);
	for (ksU32 i = 0; i < CameraManager::getNumCameras(); ++i)
	{
		Camera* cam = CameraManager::getCamera(i);

		cam->setAspectRatio(aspect);
		cam->refreshProjectionMatrix();
	}
	glViewport(0, 0, width, height);
}


void GLApplication::render_callback()
{
	TRACE_FUNC();
	Camera* cam = CameraManager::getMainCamera();

	Service<ks::GLRenderer>::Get()->render(cam->getPosition(), cam->getView(), cam->getProjection());

	Service<GLApplication>::Get()->drawTextData();

	glutSwapBuffers();
	//glutReportErrors();
}

//================================================================================================================


ks::SimpleShaderContainer* GLApplication::loadShader(const char* filename)
{
	ks::SimpleShaderContainer* shader = ks::RenderResourceFactory::findOrCreateShader(filename);
	if (shader != nullptr)
	{
		for (int i = 0; i < TOTAL_SHADER_CONSTANTS; ++i)
		{
			shader->registerConstant(gShaderConstants[i]);
		}
	}
	return shader;
}

//================================================================================================================

void GLApplication::addRenderTextData(const RenderTextDesc& pDesc)
{
	mRenderTextBuffer.push_back(pDesc);
}

//================================================================================================================

void GLApplication::drawTextData()
{
	for (RenderTextDesc& td : mRenderTextBuffer)
	{
		glRasterPos3f(td.normalised_scr_coords.x, td.normalised_scr_coords.y, 0);
		td.mat->ShaderContainer->bindProgram();
		td.mat->SetShaderParams();
		auto& buffer = td.text;
		for (int i = 0; i < RenderTextDesc::MAX_SIZE && buffer[i] != '\0'; ++i)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
	}

	mRenderTextBuffer.clear();
}