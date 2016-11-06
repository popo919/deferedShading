#version 330 core
in vec3 fragNormal;
in vec3 fragPos;
in vec2 fragUV;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAldobeSpec;

out vec4 color;

struct Meterial
{
    sampler2D texture_diffuse1;
	sampler2D texture_specular1;
    float shininess;
};

uniform Meterial material;
uniform vec3 eye;

const float Near;
const float far;
float RecoverDepth(float depth)
{
	float z = depth * 2 - 1;
	return (2.0 * Near * Far) / (Far + Near - z * (Far - Near));
}

void main()
{
	gPosition.xyz = fragPos;
	gPosition.a = RecoverDepth(fragPos.z);
	gNormal = fragNormal;
	gAldobeSpec.rgb = texture(material.texture_diffuse1, fragUV).rgb;
	gAldobeSpec.a = texture(material.texture_specular1, fragUV).r;
}

