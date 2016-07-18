//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/RenderTexture.h
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
#ifndef RENDER_TEXTURE_H_
#define RENDER_TEXTURE_H_

class RenderTexture 
{
public:
	enum DepthType 
	{
		Depth24,
		Depth16,
		NoDepth
	};
	enum ColorType 
	{
		RGB888,
		RGBA8888,
		HDR = 0xFFFF,
		RG16F,
		RGB16F,
		RGBA16F,
	};
	RenderTexture();
	~RenderTexture();
	bool init(int width, int height, ColorType color, DepthType depth);
	void activateFB();
	int getWidth() 
	{
		return mWidth;
	}
	int getHeight() 
	{
		return mHeight;
	}
	void bind();
	void release();
	unsigned int getTexId() 
	{ 
		return mTexId; 
	}
	static bool ms_useFiltering;

private:
	int mWidth;
	int mHeight;
	unsigned int mFormat;
	unsigned int mType;
	unsigned int mFboId;
	unsigned int mTexId;
	unsigned int mDepthRBId;
	unsigned int mColorRBId;
};

#endif  // RENDER_TEXTURE_H_