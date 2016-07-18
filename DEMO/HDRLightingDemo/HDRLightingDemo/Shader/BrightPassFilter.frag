#version 330 core

out vec4 color;
in vec2 TexCoords;

// The per-color weighting to be used for luminance calculations in RGB order.
const vec3 BRIGHT_PASS_THRESHOLD  	= vec3(6.0, 6.0, 6.0);
const vec3 BRIGHT_PASS_OFFSET 		= vec3(10.0, 10.0, 10.0);
uniform sampler2D quadBuffer;

void main()
{
	vec3 sample = texture2D(quadBuffer, TexCoords).rgb;
	sample -= BRIGHT_PASS_THRESHOLD;
	// Clamp to 0
	sample = max(sample, 0.0f);
	// Map the resulting value into the 0 to 1 range. Higher values for
	// BRIGHT_PASS_OFFSET will isolate lights from illuminated scene 
	// objects.
	color = vec4(sample / (BRIGHT_PASS_OFFSET + sample), 1.0);
}