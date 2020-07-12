#version 330

in vec2 uvout;
in vec3 vertlight;

out vec4 Color;

uniform sampler2D albedo_map;
uniform sampler2D emission_map;
uniform sampler2D normal_map;

uniform mat4 modelmat;
uniform mat4 viewmat;

void main()
{
    float light_pow = clamp(vertlight.r + vertlight.g + vertlight.b, 0, 1) * 1.8;

    vec4 albedodata = texture2D(albedo_map, uvout);
    vec4 emissiondata = texture2D(emission_map, uvout);
    float texalpha = albedodata.a;

    vec3 matdiffuse = albedodata.rgb;
    vec3 matambient = vec3(0.05, 0.05, 0.05) * matdiffuse;
    vec3 matemission = emissiondata.rgb;

    Color = vec4(
            matambient + 
            matdiffuse * vertlight * light_pow +
            matemission,
            texalpha);
}
