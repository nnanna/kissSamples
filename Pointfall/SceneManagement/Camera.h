#ifndef CAMERA_H
#define CAMERA_H

/************************************************************************/
//	Basic Camera Class and Static Camera Manager class.
//	Currently only points at origin.
//	Would need extending via Interfaces to support various kinds of cam.
/************************************************************************/

#include "Array.h"
#include <Maths/ks_Maths.h>

typedef ks::Matrix4x4	Matrix;
typedef ks::vec3		vec3;

class Camera
{
public:
	Camera();
	~Camera();

	const vec3&			getPosition() const		{ return mPosition; }

	void				setPosition(vec3& pos)	{ mPosition = pos; }

	const vec3&			getLookAt()	const		{ return mLookAt; }

	void				setLookAt(vec3& look)	{ mLookAt = look; }

	const Matrix&	getWorld()	const			{ return mWorld; }

	const Matrix&	getView()	const			{ return mView; }

	const Matrix&	getProjection()	const		{ return mProjection; }

	void		setFOV(float fov)				{ mFOV = fov; }

	float		getFOV()	const				{ return mFOV; }

	void		setAspectRatio(float ratio)		{ mAspectRatio = ratio; }

	float		getAspectRatio()	const		{ return mAspectRatio; }

	void		setNearClipPlane(float zNear)	{ mNear = zNear; }

	float		getNearClipPlane()	const		{ return mNear; }

	void		setFarClipPlane(float zFar)		{ mFar = zFar; }

	float		getFarClipPlane()	const		{ return mFar; }

	void		refreshProjectionMatrix();

	void		update(float elapsed);

private:

	vec3		mLookAt;

	vec3		mPosition;

	Matrix		mWorld;

	Matrix		mView;

	Matrix		mProjection;

	float		mFOV;

	float		mAspectRatio;

	float		mNear;

	float		mFar;

	float		mAngle;

	float		mRadius;
};


class CameraManager
{
public:

	static Camera*	getMainCamera();

	static Camera*	createCamera();

	static ksU32	getNumCameras()			{ return mCameras.size(); }

	static Camera*	getCamera( ksU32 i )	{ return mCameras[i];	}

	static void		update(float elapsed, bool update_all_cams);

	static void		destroy();

private:

	static Camera*				mMainCamera;

	static ks::Array<Camera*>	mCameras;
};

#endif