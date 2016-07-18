#version 330 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D sampleBuffer;
uniform int width;
uniform int height;
uniform int sampleType;

const float GAUSSIAN[5] = float[5](0.054489, 0.244201, 0.402620, 0.244201, 0.054489);


vec4 gaussBlurHori()
{
	vec4 sample = vec4(0.0);
	float tu = 1 / float(width);
	sample += GAUSSIAN[0] * texture2D(sampleBuffer, TexCoords + vec2(-2 * tu, 0));
	sample += GAUSSIAN[1] * texture2D(sampleBuffer, TexCoords + vec2(-tu, 0));
	sample += GAUSSIAN[2] * texture2D(sampleBuffer, TexCoords + vec2(0, 0));
	sample += GAUSSIAN[3] * texture2D(sampleBuffer, TexCoords + vec2(tu, 0));
	sample += GAUSSIAN[4] * texture2D(sampleBuffer, TexCoords + vec2( 2 * tu, 0));

	return sample;
}

vec4 gaussBlurVert()
{
	vec4 sample = vec4(0.0);
	float tv = 1 / float(height);
	sample += GAUSSIAN[0] * texture2D(sampleBuffer, TexCoords + vec2(0, -2 * tv));
	sample += GAUSSIAN[1] * texture2D(sampleBuffer, TexCoords + vec2(0, -tv));
	sample += GAUSSIAN[2] * texture2D(sampleBuffer, TexCoords + vec2(0, 0));
	sample += GAUSSIAN[3] * texture2D(sampleBuffer, TexCoords + vec2(0, tv));
	sample += GAUSSIAN[4] * texture2D(sampleBuffer, TexCoords + vec2(0, 2 * tv));

	return sample;
}

void main()
{
	if(sampleType == 1)
		color = gaussBlurHori();
	else if(sampleType == 2)
		color = gaussBlurVert();
}