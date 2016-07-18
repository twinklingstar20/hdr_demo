//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/RenderTexture.cpp
// SDK Version: v2.11 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#include "RenderTexture.h"
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>

#ifndef GL_RGBA16F
#define GL_RGBA16F GL_RGBA16F_ARB
#endif

bool RenderTexture::ms_useFiltering = true;

RenderTexture::RenderTexture():
	mWidth(0U),
	mHeight(0U),
	mFboId(0),
	mTexId(0),
	mDepthRBId(0),
	mColorRBId(0)
{
}

RenderTexture::~RenderTexture() 
{
	if (mTexId) 
	{
		glDeleteTextures(1, &mTexId);
		mTexId = 0;
	}
	if (mColorRBId) 
	{
		glDeleteRenderbuffers(1, &mColorRBId);
		mColorRBId = 0;
	}
	if (mDepthRBId) 
	{
		glDeleteRenderbuffers(1, &mDepthRBId);
		mDepthRBId = 0;
	}
	if (mFboId) 
	{
		glDeleteFramebuffers(1, &mFboId);
		mFboId = 0;
	}
}

void RenderTexture::bind() 
{ 
	glBindTexture(GL_TEXTURE_2D, mTexId); 
}

void RenderTexture::release() 
{ 
	glBindTexture(GL_TEXTURE_2D, 0); 
}

bool RenderTexture::init(int width, int height, ColorType color, DepthType depth)
{
	GLenum err;
	mWidth = width;
	mHeight = height;
	glGenFramebuffers(1, &mFboId);
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
	err = glGetError();

	if (color >= HDR) 
	{
		// Create the HDR render target
		switch (color) 
		{
		case RGB16F: mFormat = GL_RGB;  mType = GL_HALF_FLOAT; break;
		default:
		case RGBA16F: mFormat = GL_RGBA; mType = GL_HALF_FLOAT; break;
		}
		glGenTextures(1, &mTexId);
		glBindTexture(GL_TEXTURE_2D, mTexId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//note that internalformat and format have both to be GL_RGB or GL_RGBA for creating floating point texture.
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height); 
		err = glGetError();
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexId, 0);
		err = glGetError();
	} 
	else 
	{
		// Create the LDR render target
		mType = GL_UNSIGNED_BYTE;
		glGenTextures(1, &mTexId);
		glBindTexture(GL_TEXTURE_2D, mTexId);
		switch (color) 
		{
		case RGBA8888: mFormat = GL_RGBA; break;
		default:
		case RGB888: mFormat = GL_RGB; break;
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexStorage2D(GL_TEXTURE_2D, 1, mFormat, width, height); 

		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexId, 0);
	}
	// Create depth buffer
	if (depth != NoDepth) 
	{
		glGenRenderbuffers(1, &mDepthRBId);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthRBId);
		err = glGetError();
		unsigned int depth_format = 0;
		switch (depth) 
		{
		case Depth24: depth_format = GL_DEPTH_COMPONENT24; break;
		default:
		case Depth16: depth_format = GL_DEPTH_COMPONENT16; break;
		}
		glRenderbufferStorage(GL_RENDERBUFFER, depth_format, width, height);
		err = glGetError();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRBId);
		err = glGetError();
		glBindRenderbuffer(GL_RENDERBUFFER, 0);    
	}
	// Finalize
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
	{
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void RenderTexture::activateFB()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
	glViewport(0, 0, mWidth, mHeight);
}
