#version 330

in vec2 uvout;

out vec4 Color;

uniform sampler2D spacetex;
uniform mat4 viewmat;
uniform mat4 modelmat;

void main()
{
    vec4 matdiffuse = texture2D(spacetex, uvout).rgba;

    Color = matdiffuse;
}
