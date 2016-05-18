
#ifndef RENDERTARGET_H
#define	RENDERTARGET_H

#include "defines.h"
#include <Array.h>

typedef	ksU32		kissRenderTexture;


struct RenderTargetDesc
{
	ksU32 width;
	ksU32 height;
	ks32	hardware_format;		// GL_RGB10_A2, GL_RGBA16F_ARB etc
	bool	is_depth_buffer;		//
};


class RenderTarget
{
public:

	RenderTarget();

	~RenderTarget();

	kissRenderTexture	createOrGetRenderTarget();

	void				addRenderSurface(RenderTargetDesc desc);

private:

	kissRenderTexture				mRenderTarget;

	kissRenderTexture				mDepthBuffer;

	ks::Array<kissRenderTexture>	mRenderSurfaceCollection;
};


#endif