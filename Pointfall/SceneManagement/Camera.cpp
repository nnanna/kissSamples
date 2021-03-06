#include "Camera.h"
#include <defines.h>
#include <AppLayer/InputListener.h>


Camera::Camera() : mNear(1), mFar(90000), mAspectRatio(0.75f), mFOV(60.0f), mRadians(0.f), mRadius(30.f)
{
	mLookAt		= vec3(0,0,0);
	mPosition	= vec3(0, 5, 30);
}


void Camera::refreshProjectionMatrix()
{
	ks::buildProjectionMatrix( mFOV, mAspectRatio, mNear, mFar, mProjection.m[0] );
}


Camera::~Camera()
{}

void Camera::update(float elapsed)
{
#define ROTATION_DAMPING	0.05f
	const ksU32 downKey	= InputListener::getKeyDown();
	const float speed	= elapsed * 10.f;

	if (downKey & KEYPRESS_SHIFT)
	{
		if (downKey & KEYPRESS_DOWN)
		{
			mPosition.y -= speed;
			mLookAt.y -= speed;
		}

		if (downKey & KEYPRESS_UP)
		{
			mPosition.y += speed;
			mLookAt.y += speed;
		}
	}
	else if (downKey & KEYPRESS_CTRL)
	{
		if (downKey & KEYPRESS_UP)
			mPosition.y += speed;

		if (downKey & KEYPRESS_DOWN)
			mPosition.y -= speed;
	}
	else
	{
		if (downKey & KEYPRESS_UP)
			mRadius -= speed;

		if (downKey & KEYPRESS_DOWN)
			mRadius += speed;

		if (downKey & KEYPRESS_LEFT)
			mRadians -= speed * ROTATION_DAMPING;

		if (downKey & KEYPRESS_RIGHT)
			mRadians += speed * ROTATION_DAMPING;
	}

	mPosition = vec3(mRadius*sinf(mRadians), mPosition.y, mRadius*cosf(mRadians));


	ks::buildViewMatrix(mPosition.x, mPosition.y, mPosition.z,
					mLookAt.x, mLookAt.y, mLookAt.z,
					0, 1, 0,
					mView.m[0] );
}






//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Camera*					CameraManager::mMainCamera	= NULL;
ks::Array<Camera*>		CameraManager::mCameras		= ks::Array<Camera*>();



Camera* CameraManager::createCamera()
{
	mCameras.push_back(new Camera());
	return mMainCamera = mCameras.back();
}


Camera* CameraManager::getMainCamera()
{
	if (!mMainCamera && mCameras.empty())
	{
		return createCamera();
	}
	return mMainCamera;
}

void CameraManager::update(float elapsed, bool update_all_cams)
{
	if (update_all_cams)
	{
		for (ksU32 i = 0; i < mCameras.size(); ++i)
		{
			mCameras[i]->update(elapsed);
		}
	}
	else if (mMainCamera)
		mMainCamera->update(elapsed);
}

void CameraManager::destroy()
{
	for (ksU32 i = 0; i < mCameras.size(); i++)
	{
		SAFE_DELETE(mCameras[i]);
	}
	mCameras.clear();
}