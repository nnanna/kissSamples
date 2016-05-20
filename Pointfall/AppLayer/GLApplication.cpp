
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
#include <RenderEngine\GL\glut.h>
#include <FX\ParticleSystem.h>
#include <FX\Particles.h>
#include <Concurrency\JobScheduler.h>
#include <chrono>

typedef ks::Matrix	Matrix;
typedef ks::vec4	vec4;

using namespace std::chrono;


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


const char	*gLitShaderFilename		= "__default",
			*gBasicLitVertProgram	= "media\\programs\\basicLight_v.glsl",
			*gBasicLitFragProgram	= "media\\programs\\basicLight_f.glsl",
			*gUnlitShaderFilename	= "__unlit",
			*gUnlitVertProgram		= "media\\programs\\basicUnlit_v.glsl",
			*gUnlitFragProgram		= "media\\programs\\basicUnlit_f.glsl";

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

	glutInit(&argc, argv);
	glutInitWindowSize(600, 400);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

	glutCreateWindow(mAppName);
	glutDisplayFunc(GLApplication::render_callback);
	glutReshapeFunc(GLApplication::reshape_callback);
	glutKeyboardFunc(InputListener::KeyDownCallback);
	glutKeyboardUpFunc(InputListener::KeyUpCallback);
	glutTimerFunc( 1, GLApplication::update_callback, 1 );

	glClearColor(0.5f, 0.5f, 0.5f, 0.f);  // Gray background.
	glEnable(GL_DEPTH_TEST);

	Service<GLApplication>::Register( this );

	mRenderer = new ks::GLRenderer();
	Service<ks::GLRenderer>::Register(mRenderer);

	CameraManager::createCamera();

	mJobScheduler = new ks::JobScheduler(4, 7);		// 4 workers, 7 jobs max, multi-producer
	Service<ks::JobScheduler>::Register( mJobScheduler );

	gFontMaterial.SetDiffuse(0, 0, 0);
	gFontMaterial.ShaderContainer = loadShader(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);

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
		pSys->initMaterial(gLitShaderFilename, gBasicLitVertProgram, gBasicLitFragProgram);

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


		emz.mWorldPos = vec3(-3.f, 18.115f, 0.f);
		emz.mEmissionVelocity = vec3(0.f, -0.2492f, 0.f);
		emz.mMaxParticles = 3173;
		emz.mEmissionRate = 200;

		pSys->spawn(emz);
	}
	glPointSize( PARTICLE_GL_POINT_SIZE );

	//sceneObj->loadModel("..\\media\\models\\venusm.obj");
	//sceneObj->loadModel("..\\media\\models\\crysponza_bubbles\\sponza.obj");

	initTimer();

	Material::gConstantsRegistry = gShaderConstants;

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
	addRenderTextData(gFPSData);
	
	
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
	Camera* cam = CameraManager::getMainCamera();

	Service<ks::GLRenderer>::Get()->render(cam->getPosition(), cam->getView(), cam->getProjection());

	Service<GLApplication>::Get()->drawTextData();

	glutSwapBuffers();
	//glutReportErrors();
}

//================================================================================================================


ks::SimpleShaderContainer* GLApplication::loadShader(const char* name, const char* vp_filename, const char* fp_filename)
{
	ks::SimpleShaderContainer* shader = ks::RenderResourceFactory::findShader(name);
	if (shader == nullptr)
	{
		shader = ks::RenderResourceFactory::findOrCreateShader(name);
		shader->loadProgram(gUnlitShaderFilename, gUnlitVertProgram, gUnlitFragProgram);

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