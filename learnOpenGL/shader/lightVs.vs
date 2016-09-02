#versio n 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in mat4 model;

uniform mat4 VP;

void main()
{
gl_Position = VP * model * vec4(position.x, position.y, position.z, 1.0);
}