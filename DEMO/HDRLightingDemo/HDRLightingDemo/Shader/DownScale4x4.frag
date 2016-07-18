#version 330 core

out vec4 color;
in vec2 TexCoords;

const int MAX_SAMPLES = 16;

uniform sampler2D quadBuffer;
uniform vec2 samples[MAX_SAMPLES];
uniform int width;
uniform int height;

void main()
{
	vec3 sample = vec3(0.0f);
	int x, y;
	vec2 offset;
	float tu = 1 / float(width), tv = 1 / float(height);
	for(y = 0; y < 4 ; y ++)
	{
		for(x = 0; x < 4; x ++)
		{
			offset = vec2((x - 1.5) * tu, (y - 1.5) * tv);
			sample += texture2D(quadBuffer, TexCoords + offset).rgb;
		}
	}
	color = vec4(sample / 16, 1.0);
}