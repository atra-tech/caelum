#version 330

in vec2 uvout;

out vec4 Color;

uniform sampler2D albedo_map;

uniform mat4 modelmat;
uniform mat4 viewmat;

void main()
{
    vec4 albedodata = texture2D(albedo_map, uvout);
    float texalpha = albedodata.a;

    vec3 matdiffuse = albedodata.rgb;
    vec3 matambient = vec3(0.05, 0.05, 0.05) * matdiffuse;

    Color = vec4(matambient + matdiffuse, texalpha);
}
