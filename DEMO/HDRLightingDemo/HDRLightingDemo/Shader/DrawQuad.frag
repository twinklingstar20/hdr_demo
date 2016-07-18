#version 330 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D hdrBuffer;

void main()
{             
	const float gamma = 2.2;
	color = vec4(pow(texture(hdrBuffer, TexCoords).rgb, vec3(1.0 / gamma)), 1.0);
}