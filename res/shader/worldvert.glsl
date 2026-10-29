#version 330 core

const int MAX_LIGHTS = 8;

layout (location = 0) in vec3 verts;
layout (location = 1) in vec2 uvs;
layout (location = 2) in vec3 norms;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform mat4 projviewmat;
uniform mat4 viewmat;
uniform mat4 modelmat;

uniform vec3 light_pos[MAX_LIGHTS];
uniform int light_count;

uniform vec3 viewpos;

out VS_OUT {
    vec3 frag_pos;
    vec2 tex_coords;
    vec3 tangent_lightpos[MAX_LIGHTS];
    vec3 tangent_viewpos;
    vec3 tangent_fragpos;
} vs_out;

out vec2 uvout;

out vec2 uvmin;
out vec2 uvmax;


float img_size = 1024.0;
float tex_size = 32.0;
float pix_size = 1.0 / img_size;

float offset = pix_size * 0.001;

float tex_size_uv = tex_size / img_size;
float tex_pos_u = floor(uvs[0] / tex_size_uv) * tex_size_uv;
float tex_pos_v = floor(uvs[1] / tex_size_uv) * tex_size_uv;


void
main()
{
    vs_out.frag_pos = vec3(modelmat * vec4(verts, 1.0));
    vs_out.tex_coords = uvs;

    mat3 normalmat = transpose(inverse(mat3(modelmat)));
    vec3 T = normalize(normalmat * tangent);
    vec3 N = normalize(normalmat * norms);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));

    for (int i = 0; i < light_count; i++) {
        vs_out.tangent_lightpos[i] = TBN * light_pos[i];
    }
    vs_out.tangent_viewpos = TBN * viewpos;
    vs_out.tangent_fragpos = TBN * vs_out.frag_pos;

    uvout = uvs;

    uvmin = vec2(tex_pos_u + offset, tex_pos_v + offset);
    uvmax = uvmin + vec2(tex_size_uv - 2 * offset, tex_size_uv - 2 * offset);
    
    gl_Position = projviewmat * modelmat * vec4(verts, 1.0);
}
