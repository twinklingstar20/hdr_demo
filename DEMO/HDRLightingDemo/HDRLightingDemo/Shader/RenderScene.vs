#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out VS_OUT
{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoords;
} vs_out;

uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main()
{
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0f);
	vs_out.fragPos = vec3(modelMatrix * vec4(position, 1.0));   
	vs_out.texCoords = texCoords;
	vs_out.normal = normalize(mat3(modelMatrix) * normal);
}