#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 MVP;
uniform mat4 Model;

void main()
{
fragNormal = normalize(normal);
fragPos = vec3(Model * vec4(position, 1));
fragUV = uv;
gl_Position = MVP * vec4(position.x, position.y, position.z, 1.0);
}