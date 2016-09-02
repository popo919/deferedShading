#version 330 core

in vec2 quadUV;

layout (location = 0) out vec4 ouputColor;

struct Light
{
	vec3 color;
	vec3 direction;
	vec4 position;
	vec3 ambient;
	float angle;
	float outerAngle;

	float constant;
	float linear;
	float quadratic;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform Light light;
uniform vec3 eye;

vec3 pointLightShading(Light light, vec3 fragPos, vec3 fragNormal, vec3 spec, vec3 diff);
vec3 directionalLightShading(Light light, vec3 fragPos, vec3 fragNormal, vec3 spec, vec3 diff);

void main()
{
	vec3 fragPos = texture(gPosition, quadUV).xyz;
	vec3 fragNormal = texture(gNormal, quadUV).xyz;
	vec3 diffuse = texture(gAlbedoSpec, quadUV).rgb;
	float spec = 1 - texture(gAlbedoSpec, quadUV).a;
	vec3 specular = vec3(spec, spec, spec);

	vec3 res = directionalLightShading(light, fragPos, fragNormal, specular, diffuse);
	//ouputColor = vec4(res, 1.0f);
	ouputColor = vec4(res, 1);
}

vec3 pointLightShading(Light light, vec3 fragPos, vec3 fragNormal, vec3 spec, vec3 diff)
{
	float distance = length(light.position.xyz - fragPos.xyz);
	vec3 lightDir = normalize(light.position.xyz - fragPos.xyz);
	vec3 eyeDir = normalize(eye - fragPos.xyz); 
	vec3 reflection = normalize(reflect(-lightDir, fragNormal));
	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * distance * distance);

	vec3 ambient = light.ambient * diff;
	vec3 diffuse = light.color * max(dot(lightDir, fragNormal), 0.0) * diff;
	vec3 specular = light.color * 
		pow(max(dot(eyeDir, reflection), 0.0), 32) * spec;
	
	vec3 color = ambient + diffuse * attenuation + specular * attenuation;
	return color;
}

vec3 directionalLightShading(Light light, vec3 fragPos, vec3 fragNormal, vec3 spec, vec3 diff)
{
	vec3 lightDir = light.direction.xyz;
	vec3 eyeDir = normalize(eye - fragPos.xyz);
	vec3 reflection = normalize(reflect(-lightDir, fragNormal));

	vec3 ambient = light.ambient * diff;
	vec3 diffuse = light.color * max(dot(lightDir, fragNormal), 0.0) * diff;
	vec3 specular = light.color * 
		pow(max(dot(eyeDir, reflection), 0.0), 16) * spec;
	
	vec3 color = ambient + diffuse + specular;
	return color;
}
