
#include "RenderTarget.h"
#include "GL/glew.h"
#include "ErrorNotify.h"

RenderTarget::RenderTarget() : mRenderTarget(NULL)
{

}

RenderTarget::~RenderTarget()
{
	if (mRenderTarget)
	{
		glDeleteFramebuffersEXT(1, &mRenderTarget);
		for (ksU32 i = 0; i < mRenderSurfaceCollection.size(); i++)
		{
			glDeleteTextures(1, &mRenderSurfaceCollection[i]);
		}
		mRenderSurfaceCollection.clear();
		mRenderTarget = NULL;
	}
}


kissRenderTexture RenderTarget::createOrGetRenderTarget()
{
	if (!mRenderTarget)
	{
		glGenFramebuffersEXT(1, &mRenderTarget);
	}
	
	return mRenderTarget;
}


void RenderTarget::addRenderSurface(RenderTargetDesc desc)
{
	//
	// this is minimalist as a lot of the setup is done with hard-coded values. will grow flexible with time lol.
	//
	createOrGetRenderTarget();
	glBindFramebufferEXT(1, mRenderTarget);

	kissRenderTexture texture = NULL;
	glGenTextures( 1, &texture);
	glBindTexture( GL_TEXTURE_2D, texture);

	glTexImage2D( GL_TEXTURE_2D, 0, desc.hardware_format/*GL_RGB10_A2*/, desc.width, desc.height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (desc.is_depth_buffer)
	{
		mDepthBuffer = texture;
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, texture, 0);
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, texture, 0);
	}
	else
	{
		ksU32 i = mRenderSurfaceCollection.size();
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_TEXTURE_2D, texture, 0);
		mRenderSurfaceCollection.push_back( std::move(texture) );
	}

	// unbind texture
	glBindTexture(1, NULL);

	if ( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		ErrorNotify("bad frame buffer in addRenderSurface");
}