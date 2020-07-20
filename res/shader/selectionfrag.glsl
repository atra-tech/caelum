#version 330

in vec2 uvout;
in vec3 pos;

out vec4 Color;

uniform sampler2D selectiontex;
uniform mat4 viewmat;
uniform mat4 modelmat;

void main()
{
    vec3 selectcolor = vec3(1.0, 0.7, 0.0);

    float alpha_x = cos(pos.x * 6.28) - 0.5;
    float alpha_z = cos(pos.z * 6.28) - 0.5;
    float alpha_y = cos(clamp(pos.y * 3.14, 0, 3.14));// + 0.5;

    float alpha = (alpha_x + alpha_y + alpha_z) * 0.5;

    Color = vec4(selectcolor * 1.5, alpha);
}
