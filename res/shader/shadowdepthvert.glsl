#version 330

layout (location) in vec3 verts;

uniform mat4 lightspacemat;
uniform mat4 modelmat;

void main()
{
    gl_Position = lightspacemat * model * vec4(verts, 1.0);
}
