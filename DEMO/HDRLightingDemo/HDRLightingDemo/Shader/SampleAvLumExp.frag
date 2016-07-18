#version 330 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D quadBuffer;
uniform int width;
uniform int height;

void main()
{
	float logLumSum = 0.0;
	vec2 offset;
	int x, y;
	float tu = 1.0 / float(width), tv = 1.0 / float(height);
	for(x = 0; x < 4 ; x++)
	{
		for(y = 0; y < 4; y++)
		{
			offset = vec2(float(x - 1.5) * tu, float(y - 1.5) * tv);
			logLumSum += texture2D(quadBuffer, TexCoords + offset).r;
		}
	}
	logLumSum = exp(logLumSum / 16);
	color = vec4(logLumSum, logLumSum, logLumSum, 1.0);
}