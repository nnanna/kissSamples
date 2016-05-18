/*
	Nnanna Kama
	Provides basic functionality for starting up an OpenGL app
	Methods can be overloaded in derived class
*/

#pragma once

#include "defines.h"
#include <Array.h>


class ParticleSystem;
class SceneObject;
class GLRenderer;
namespace ks
{
	class JobScheduler;
}



class GLApplication
{
public:

	GLApplication(void);

	virtual ~GLApplication(void);

	/*	Initiliaze all that's needed for app to run. Returns false on fail */
	virtual bool init(int argc, char** argv);
	
	
	/*	Kicks off app loop.*/
	virtual void go();

	
	/* Update loop for registered objects/systems */
	virtual void update(int);


	template<class T>
	ksU32 registerObject(T* o)
	{
		ksU32 i(0);
		ksU32 id			= o->getUID();
		auto& objects		= getObjectCollection<T>();

		for ( i = 0; i < objects.size() && o != objects[i]; ++i )
		{}

		bool not_registered = (i >= objects.size() );

		if ( not_registered )
		{
			objects.push_back(o);
			id = mRegisteredItemsCount++;
		}

		return id;
	}


	/*	Destroy application*/
	static void destroy(int);

	
	/*	clean up after app and exit	*/
	virtual void quit();

	
	/* update callback handle for glut loop */
	static void update_callback(int);




protected:

	template<class T>
	ks::Array<T*>&		getObjectCollection();

	GLRenderer*			mRenderer;

	ks::JobScheduler*	mJobScheduler;

	char				mAppName[MAX_NAME];			// Application name

	bool				mIsFullscreen;				// fullscreen toggle. unused.

	int					mRegisteredItemsCount;		// used to maintain unique IDs and index registered objects

	double				mElapsedS;

	
	/*
		Dynamic arrays to the App's subsystems. Just ParticleSystem for now
	*/

	ks::Array<ParticleSystem*>	mParticleSubsytems;

	ks::Array<SceneObject*>		mSceneObjects;

};






