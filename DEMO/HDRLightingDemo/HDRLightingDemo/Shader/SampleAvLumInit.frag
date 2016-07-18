#version 330 core

out vec4 color;
in vec2 TexCoords;

// The per-color weighting to be used for luminance calculations in RGB order.
const vec3 LUMINANCE_VECTOR  = vec3(0.2125f, 0.7154f, 0.0721f);

uniform sampler2D quadBuffer;
uniform int width;
uniform int height;

void main()
{
	vec3 sample;
	float logLumSum = 0.0;
	vec2 offset;
	int x, y;
	float tu = 1.0 / float(3 * width), tv = 1.0 / float(3 * height);
	for(x = -1; x <= 1 ; x++)
	{
		for(y = -1; y <= 1; y++)
		{
			offset = vec2(float(x) * tu, float(y) * tv);
			sample = texture2D(quadBuffer, TexCoords + offset).rgb;
			logLumSum += log(dot(sample, LUMINANCE_VECTOR) + 0.0001f);
		}
	}
	logLumSum /= 9;
	color = vec4(logLumSum, logLumSum, logLumSum, 1.0);
}