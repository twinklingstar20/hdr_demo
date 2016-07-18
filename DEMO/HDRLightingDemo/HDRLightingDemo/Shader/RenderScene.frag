#version 330 core

out vec4 fragColor;

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
} fs_in;

struct Light {
    vec3 position;
    vec3 color;
};

const int LIGHT_NUMS = 2;

uniform Light lights[2];
uniform float phongCoeff;
uniform float phongExp;
uniform float diffCoeff;
uniform sampler2D diffuseTexture;

void main()
{           
    vec3 color = texture(diffuseTexture, fs_in.texCoords).rgb;
    vec3 n = normalize(fs_in.normal);
    // Ambient
    float amb = 0.05;
    // Lighting
    vec3 lighting = vec3(0.0f);
    for(int i = 0; i < 2; i++)
    {
        // Diffuse
        vec3 l = normalize(lights[i].position - fs_in.fragPos);
        // because of the floating frame buffer, clamp() hear.
        float diff = max(dot(l, n), 0.0);
        float spec = max(dot(reflect(-l, n), n), 0.0);
        vec3 res = lights[i].color * (diff + amb + phongCoeff * pow(spec, phongExp)) * color;     
        // Attenuation (use quadratic as we have gamma correction)
        float d = length(fs_in.fragPos - lights[i].position);
        res *= 1.0 / (d * d);
        lighting += res;
    }
    //lighting = vec3(1.0f, 0.0f, 0.0f);
    fragColor = vec4(lighting, 1.0f);
}