#version 330 core

const int MAX_LIGHTS = 8;
const int LIGHTMAP_SIZE = 2048;
const int LIGHTMAP_CELL_SIZE = 64;

const float TEXATLAS_SIZE = 1024.0;
const float TEXATLAS_SUBTEX_SIZE = 128.0;
const float TEXATLAS_SUBTEX_COUNT = TEXATLAS_SIZE / TEXATLAS_SUBTEX_SIZE;
const float TEXATLAS_PIXSIZE = 1.0 / TEXATLAS_SIZE;
const float TEXATLAS_SUBTEX_PIXSIZE = 1.0 / TEXATLAS_SUBTEX_SIZE;
const float TEXATLAS_SUBTEX_COUNT_INV = 1.0 / TEXATLAS_SUBTEX_COUNT;

in vec2 uvout;

in vec2 uvmin;
in vec2 uvmax;

in VS_OUT {
    vec3 frag_pos;
    vec2 tex_coords;
    vec3 tangent_lightpos[MAX_LIGHTS];
    vec3 tangent_viewpos;
    vec3 tangent_fragpos;
} fs_in;

out vec4 Color;

uniform sampler2D albedo_map;
uniform sampler2D emission_map;
uniform sampler2D normal_map;
uniform sampler2D light_map;

uniform mat4 modelmat;
uniform mat4 viewmat;

uniform vec3 light_pos[MAX_LIGHTS];
uniform vec3 light_color[MAX_LIGHTS];
uniform int light_count;

uniform vec3 viewpos;

float img_w = 1024.0;
float img_h = 1024.0;

float pix_w = 1.0 / img_w;
float pix_h = 1.0 / img_h;

vec4
texture2D_bilinear(sampler2D tex_sampler, vec2 tex_coord)
{
    //vec2 subtex_pos = tex_coord * TEXATLAS_SUBTEX_COUNT;
    //vec2 subtex_min = floor(uvmin * TEXATLAS_SUBTEX_COUNT);

    float offset = TEXATLAS_PIXSIZE * 0.5;
    float sub_offset = TEXATLAS_SUBTEX_PIXSIZE * 0.5;

    float x0 = clamp(tex_coord[0] - offset, uvmin[0], uvmax[0]);
    float y0 = clamp(tex_coord[1] - offset, uvmin[1], uvmax[1]);

    float x1 = clamp(tex_coord[0] + offset, uvmin[0], uvmax[0]);
    float y1 = clamp(tex_coord[1] + offset, uvmin[1], uvmax[1]);
    
    //float x0 = (subtex_min[0] + fract(subtex_pos[0] - sub_offset)) * TEXATLAS_SUBTEX_COUNT_INV;
    //float y0 = (subtex_min[1] + fract(subtex_pos[1] - sub_offset)) * TEXATLAS_SUBTEX_COUNT_INV;

    //float x1 = (subtex_min[0] + fract(subtex_pos[0] + sub_offset)) * TEXATLAS_SUBTEX_COUNT_INV;
    //float y1 = (subtex_min[1] + fract(subtex_pos[1] + sub_offset)) * TEXATLAS_SUBTEX_COUNT_INV;

    // float x1 = clamp(x0 + pix_w, uvmin[0], uvmax[0]);
    // float y1 = clamp(y0 + pix_h, uvmin[1], uvmax[1]);

    vec4 x0y0 = texture2D(tex_sampler, vec2(x0, y0));
    vec4 x1y0 = texture2D(tex_sampler, vec2(x1, y0));
    vec4 x0y1 = texture2D(tex_sampler, vec2(x0, y1));
    vec4 x1y1 = texture2D(tex_sampler, vec2(x1, y1));

    float a = fract((tex_coord[0] - offset) * img_w);

    vec4 lerp_y0 = vec4(
            mix(x0y0.r, x1y0.r, a),
            mix(x0y0.g, x1y0.g, a),
            mix(x0y0.b, x1y0.b, a),
            mix(x0y0.a, x1y0.a, a));

    vec4 lerp_y1 = vec4(
            mix(x0y1.r, x1y1.r, a),
            mix(x0y1.g, x1y1.g, a),
            mix(x0y1.b, x1y1.b, a),
            mix(x0y1.a, x1y1.a, a));

    float b = fract((tex_coord[1] - offset) * img_h);

    return vec4(
            mix(lerp_y0.r, lerp_y1.r, b),
            mix(lerp_y0.g, lerp_y1.g, b),
            mix(lerp_y0.b, lerp_y1.b, b),
            mix(lerp_y0.a, lerp_y1.a, b));
}

vec3
calc_light(int light_id, vec3 albedo_rgb, vec3 normal_rgb)
{
    vec3 lightdir = normalize(fs_in.tangent_lightpos[light_id] - fs_in.tangent_fragpos);
    float diff = max(dot(lightdir, normal_rgb), 0.0);
    
    vec3 viewdir = normalize(fs_in.tangent_viewpos - fs_in.tangent_fragpos);
    vec3 reflectdir = reflect(-lightdir, normal_rgb);
    vec3 halfwaydir = normalize(lightdir + viewdir);
    float spec = pow(max(dot(normal_rgb, halfwaydir), 0.0), 32.0);

    const float l_const = 1.0;
    const float l_quad = 0.0000001;

    float distance = length(light_pos[light_id] - fs_in.frag_pos);
    float attenuation = 1.0 / (l_const + l_quad * pow(distance, 10));

    float lmap_pix_size = 1.0 / LIGHTMAP_SIZE;
    float lmap_cell_size = lmap_pix_size * LIGHTMAP_CELL_SIZE;
    float half_pix = lmap_pix_size / 2;

    float cell_x = (int(light_pos[light_id][0]) + 8) * LIGHTMAP_CELL_SIZE * lmap_pix_size;
    float cell_z = (int(light_pos[light_id][2]) + 8) * LIGHTMAP_CELL_SIZE * lmap_pix_size;

    float frag_x = (fs_in.frag_pos[0] - light_pos[light_id][0] + 8) * 4 * lmap_pix_size;
    float frag_z = (fs_in.frag_pos[2] - light_pos[light_id][2] + 8) * 4 * lmap_pix_size;

    vec2 pix_pos = vec2(cell_x + frag_x - half_pix, cell_z + frag_z - half_pix);

    vec3 light = texture2D(light_map, pix_pos).rgb * light_color[light_id];
    //float avglight = (light.r + light.g + light.b) / 3.0;
    //light = vec3(avglight);

    vec3 lightpow = vec3(1.5);
    light = light * lightpow;

    //vec3 ambience = light * vec3(0.2) * albedo_rgb * attenuation;
    vec3 diffuse = light * diff * albedo_rgb * attenuation;
    vec3 specular = light * spec * vec3(0.5) * attenuation;

    return (diffuse + specular);
}

void
main()
{
    //float light_pow = clamp(vertlight.r + vertlight.g + vertlight.b, 0, 1) * 1.8;

    vec4 albedodata = texture2D_bilinear(albedo_map, fs_in.tex_coords);
    vec4 emissiondata = texture2D_bilinear(emission_map, fs_in.tex_coords);
    vec3 normal = texture2D_bilinear(normal_map, fs_in.tex_coords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    float texalpha = albedodata.a;
    vec3 matdiffuse = albedodata.rgb;
    vec3 matemission = emissiondata.rgb;

    vec3 result = vec3(0.0);

    for (int i = 0; i < light_count; i++) {
        float distance = length(light_pos[i] - fs_in.frag_pos);
        
        if (distance < 8) {
            result += calc_light(i, matdiffuse, normal);
        }
    }

    vec3 ambience = vec3(0.05) * matdiffuse;

    Color = vec4(result + matemission + ambience, texalpha);
}
