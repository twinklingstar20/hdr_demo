#version 330 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D sampleBuffer;

uniform int width;
uniform int height;
uniform int sampleWidth;
uniform int sampleHeight;
uniform int sampleType;

const float PI = 3.141592653589;

float gaussianDistribution( float x, float y, float rho)
{
	float g = 1.0f / sqrt( 2.0f * PI * rho * rho );
	g *= exp( -( x * x + y * y ) / ( 2 * rho * rho));
	return g;
}

vec4 gaussBlurOrg(int smW, int smH, int bW, int bH)
{
	int i, j;
	float sum = 0.0;
	float weight;
	vec2 offset;
	int w = smW / 2, h = smH / 2;
	vec4 sample = vec4(0.0);
	float tu = 1 / float(bW), tv = 1 / float(bH);
	for(i = - w; i <= w ; i ++)
	{
		for(j = -h; j <= h; j ++)
		{
			offset = vec2(tu * i, tv * j);
			weight = gaussianDistribution(i, j, 1.0);
			sample += weight * texture2D(sampleBuffer, TexCoords + offset);
			sum += weight;
		}
	}
	sample /= sum;

	return sample;
}

vec4 gaussBlurVert(int smH, int bH)
{
	int i;
	float sum = 0.0;
	float weight;
	vec2 offset;
	int h = smH / 2;
	vec4 sample = vec4(0.0);
	float tv = 1 / float(bH);
	for(i = - h; i <= h ; i ++)
	{
		offset = vec2(0, tv * i);
		weight = gaussianDistribution(0, i, 1.0);
		sample += weight * texture2D(sampleBuffer, TexCoords + offset);
		sum += weight;
	}
	sample /= sum;

	return sample;
}

vec4 gaussBlurHori(int smW, int bW)
{
	int i;
	float sum = 0.0;
	float weight;
	vec2 offset;
	int h = smW / 2;
	vec4 sample = vec4(0.0);
	float tu = 1 / float(bW);
	for(i = - h; i <= h ; i ++)
	{
		offset = vec2(tu * i, 0);
		weight = gaussianDistribution(i, 0, 1.0);
		sample += weight * texture2D(sampleBuffer, TexCoords + offset);
		sum += weight;
	}
	sample /= sum;

	return sample;
}

void main()
{
	if(sampleType == 0)
		color = gaussBlurOrg(sampleWidth, sampleHeight, width, height);
	else if(sampleType == 1)
		color = gaussBlurHori(sampleWidth, width);
	else if(sampleType == 2)
		color = gaussBlurVert(sampleHeight, height);
}