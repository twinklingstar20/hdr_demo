#version 330 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D sceneBuffer;
uniform sampler2D bloomBuffer;
uniform sampler2D hdrBuffer;
uniform int useHdr;
uniform int useBloom;

void main()
{
	vec3 sample = texture2D(sceneBuffer, TexCoords).rgb;
	vec3 bloom = texture2D(bloomBuffer, TexCoords).rgb;
	if(useHdr == 1)
	{
		float adapt = texture2D(hdrBuffer, vec2(0.5, 0.5)).r;
		sample /= adapt;
		sample /= (vec3(1.0) + sample);
		
	}
	sample *= 0.8;
	if(useBloom == 1)
	{
		sample += bloom;
	}
	color = vec4(sample, 1.0);
}