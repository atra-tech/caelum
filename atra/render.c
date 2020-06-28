/*
 *  ATRA ENGINE (2019)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Description:
 *  A collection of rendering tools, buffers, shaders, etc.
 *
 *  */

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>

#include "render.h"
#include "util/htable.h"
#include "util/log.h"
#include "util/utf.h"

static GLuint shadercompile(const char *path, GLenum type); 
static GLuint shaderlink(GLuint vertshaderid, GLuint fragshaderid);
static void shadergetuniforms(Shader *shader);

//static List *texturefiles;
//static List *texturedata;
//static List *fontfiles;
//static List *fontdata;
static HTable *textures;
static HTable *fonts;

void
vbufinit(VBuffer *buf)
{
    buf->vao = 0;
    buf->vbocount = 0;
    buf->indxcount = 0;

    buf->buffers = listcreate();
    buf->verts = listcreate();
    buf->uvs = listcreate();
    buf->norms = listcreate();
    buf->indxs = listcreate();
}

void
vbufaddvertex(VBuffer *buf, vec3 vertex, vec2 uv, vec3 normal)
{
    float *v = malloc(3 * sizeof(float));
    float *u = malloc(2 * sizeof(float));
    float *n = malloc(3 * sizeof(float));
/*
    memcpy(v, &vertex, 3 * sizeof(float));
    memcpy(u, &uv, 2 * sizeof(float));
    memcpy(n, &normal, 3 * sizeof(float));
*/
    v[0] = vertex[0]; v[1] = vertex[1]; v[2] = vertex[2];
    u[0] = uv[0]; u[1] = uv[1];
    n[0] = normal[0]; n[1] = normal[1]; n[2] = normal[2];

    listpushback(buf->verts, v);
    listpushback(buf->uvs, u);
    listpushback(buf->norms, n);
}

void
vbufaddindex(VBuffer *buf, ivec3 index)
{
    int *i = (int *)malloc(3 * sizeof(int));

    i[0] = index[0]; i[1] = index[1]; i[2] = index[2];

    listpushback(buf->indxs, i);
}

void
vbufbindvbo(VBuffer *buf, int dimens, List *data)
{
    int i, j;
    GLuint *vbo = malloc(sizeof(GLuint));
    float *vec = malloc(data->size * dimens * sizeof(float));

    if (dimens == 2) {
        for (i = 0, j = 0; i < data->size * 2; i += 2, j++) {
            vec[i] = ((float *)data->elem[j])[0];
            vec[i+1] = ((float *)data->elem[j])[1];
        }
    } else if (dimens == 3) {
        for (i = 0, j = 0; i < data->size * 3; i += 3, j++) {
            vec[i] = ((float *)data->elem[j])[0];
            vec[i+1] = ((float *)data->elem[j])[1];
            vec[i+2] = ((float *)data->elem[j])[2];
        }
    } else if (dimens == 4) {
        for (i = 0, j = 0; i < data->size * 4; i += 4, j++) {
            vec[i] = ((float *)data->elem[j])[0];
            vec[i+1] = ((float *)data->elem[j])[1];
            vec[i+2] = ((float *)data->elem[j])[2];
            vec[i+3] = ((float *)data->elem[j])[3];
        }
    }

    if (checkmalloc(vbo) < 0) {
        logprint(LOG_ERROR, "buffer: Unable to bind VBO: " \
                            "Memory error");
        return;
    }

    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 data->size * sizeof(float) * dimens,
                 vec,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(buf->vbocount,
                          dimens,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          (GLvoid*)0);
    glEnableVertexAttribArray(buf->vbocount++);
    listpushback(buf->buffers, vbo);
    free(vec);
}

void
vbufbindebo(VBuffer *buf)
{
    int i, j;
    GLuint *ebo = malloc(sizeof(GLuint));
    int *vec = malloc(buf->indxs->size * 3 * sizeof(int));

    for (i = 0, j = 0; i < buf->indxs->size * 3; i += 3, j++) {
        vec[i+0] = ((int *)buf->indxs->elem[j])[0];
        vec[i+1] = ((int *)buf->indxs->elem[j])[1];
        vec[i+2] = ((int *)buf->indxs->elem[j])[2];
    }

    if (checkmalloc(ebo) < 0) {
        logprint(LOG_ERROR, "buffer: Unable to bind EBO: " \
                            "Memory error");
        return;
    }
   
    glGenBuffers(1, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 buf->indxs->size * sizeof(int) * 3,
                 vec,
                 GL_STATIC_DRAW);
    listpushback(buf->buffers, ebo);
    free(vec);
}

void
vbufbindvao(VBuffer *buf)
{
    glBindVertexArray(buf->vao);
}

void
vbufbind(VBuffer *buf)
{
    if (buf->vao != 0)
        vbufreset(buf);

    glGenVertexArrays(1, &buf->vao);
    vbufbindvao(buf);
    vbufbindvbo(buf, 3, buf->verts);
    vbufbindvbo(buf, 2, buf->uvs);
    vbufbindvbo(buf, 3, buf->norms);
    vbufbindebo(buf);
}

void
vbufclear(VBuffer *buf)
{
    listdealloc(buf->verts);
    listdealloc(buf->uvs);
    listdealloc(buf->norms);
    listdealloc(buf->indxs);
    listclear(buf->verts);
    listclear(buf->uvs);
    listclear(buf->norms);
    listclear(buf->indxs);
    buf->indxcount = 0;
}

void
vbufreset(VBuffer *buf)
{
    int i;
    glDeleteVertexArrays(1, &buf->vao);
    
    for (i = 0; i < buf->buffers->size; i++) {
        glDeleteBuffers(1, listat(buf->buffers, i));
    }
 
    listdealloc(buf->buffers);
    listdealloc(buf->verts);
    listdealloc(buf->uvs);
    listdealloc(buf->norms);
    listdealloc(buf->indxs);
    listclear(buf->buffers);
    listclear(buf->verts);
    listclear(buf->uvs);
    listclear(buf->norms);
    listclear(buf->indxs);

    buf->vao = 0;
    buf->vbocount = 0;
    buf->indxcount = 0;
}

void
vbufdelete(VBuffer *buf)
{
    int i;
    glDeleteVertexArrays(1, &buf->vao);

    for (i = 0; i < buf->buffers->size; i++) {
        glDeleteBuffers(1, listat(buf->buffers, i));
    }

    listdealloc(buf->buffers);
    listdealloc(buf->verts);
    listdealloc(buf->uvs);
    listdealloc(buf->norms);
    listdealloc(buf->indxs);
    listdelete(buf->buffers);
    listdelete(buf->verts);
    listdelete(buf->uvs);
    listdelete(buf->norms);
    listdelete(buf->indxs);

    buf->vao = 0;
    buf->vbocount = 0;
    buf->indxcount = 0;
}

void
shaderinit(Shader *shader, const char *vertsrc, const char *fragsrc)
{
    shader->prog = shaderload(vertsrc, fragsrc);
    shadergetuniforms(shader);
}

void
shaderdelete(Shader *shader)
{
    glDeleteProgram(shader->prog);
}

GLuint
shaderload(const char *vertpath, const char *fragpath)
{
    GLuint vertshaderid = shadercompile(vertpath, GL_VERTEX_SHADER);
    GLuint fragshaderid = shadercompile(fragpath, GL_FRAGMENT_SHADER);
    GLuint progid = shaderlink(vertshaderid, fragshaderid);
    return progid;
}

GLuint
shadercompile(const char *path, GLenum type)
{
    GLuint shaderid = glCreateShader(type);
    char *code = loadfile(path);
    const char *data = code;
    GLint result = GL_FALSE;
    int loglength = 0;

    logprint(LOG_INFO, "Compiling shader: $s", path);
    glShaderSource(shaderid, 1, &data, NULL);
    glCompileShader(shaderid);

    glGetShaderiv(shaderid, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shaderid, GL_INFO_LOG_LENGTH, &loglength);

    if (loglength > 0) {
        char *info = calloc(loglength, sizeof(char));
        glGetProgramInfoLog(shaderid, loglength, NULL, info);
        logprint(LOG_ERROR, "shader: Unable to compile shader: $s", info);
        free(info);
    }
    free(code);

    return shaderid;
}

GLuint
shaderlink(GLuint vertshaderid, GLuint fragshaderid)
{
    GLuint progid = glCreateProgram();
    GLint result = GL_FALSE;
    int loglength = 0;

    logprint(LOG_INFO, "Linking program");
    glAttachShader(progid, vertshaderid);
    glAttachShader(progid, fragshaderid);
    glLinkProgram(progid);

    glGetShaderiv(progid, GL_LINK_STATUS, &result);
    glGetShaderiv(progid, GL_INFO_LOG_LENGTH, &loglength);

    if (loglength > 0) {
        char *info = calloc(loglength, sizeof(char));
        glGetProgramInfoLog(progid, loglength, NULL, info);
        logprint(LOG_ERROR, "shader: Unable to link shaders: $s", info);
        free(info);
    }
    glDetachShader(progid, vertshaderid);
    glDetachShader(progid, fragshaderid);
    glDeleteShader(vertshaderid);
    glDeleteShader(fragshaderid);

    return progid;
}

void
shaderbind(Shader *shader)
{
    glUseProgram(shader->prog);
}

void
shaderunbind()
{
    glUseProgram(0);
}
void
shaderloadint(uint loc, int data)
{
    glUniform1i(loc, data);
}

void
shaderloadfloat(uint loc, float data)
{
    glUniform1f(loc, data);
}

void
shaderloadvec2(uint loc, vec2 vec)
{
    glUniform2f(loc, vec[0], vec[1]);
}

void
shaderloadvec3(uint loc, vec3 vec)
{
    glUniform3f(loc, vec[0], vec[1], vec[2]);
}

void
shaderloadvec4(uint loc, vec4 vec)
{

    glUniform4f(loc, vec[0], vec[1], vec[2], vec[3]);
}

void
shaderloadmat4(uint loc, mat4 mat)
{
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat[0]);
}

void
shaderloadprojviewmat(Shader *shader, mat4 matrix)
{
    shaderloadmat4(shader->projviewmat, matrix);
}

void
shaderloadmodelmat(Shader *shader, mat4 matrix)
{
    shaderloadmat4(shader->modelmat, matrix);
}

void
shaderloadviewmat(Shader *shader, mat4 matrix)
{
    shaderloadmat4(shader->viewmat, matrix);
}

void
shadergetuniforms(Shader *shader)
{
    shaderbind(shader);

    shader->projviewmat = glGetUniformLocation(shader->prog, "projviewmat");
    shader->modelmat = glGetUniformLocation(shader->prog, "modelmat");
    shader->viewmat = glGetUniformLocation(shader->prog, "viewmat");
}

void
texinit(void)
{
    //texturefiles = listcreate();
    //texturedata = listcreate();
    textures = htablecreate(htablestringhash, htablestringequal);
}

void
texload(Texture *tex, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    int mode = GL_RGB;

    if (!surface) {
        logprint(LOG_ERROR, "texture: Unable to load texture file $s: " \
                            "SDL_image error: $s", path, IMG_GetError());
        return;
    }
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    if (surface->format->BytesPerPixel == 4) {
        mode = GL_RGBA;
    }
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 mode,
                 surface->w,
                 surface->h,
                 0,
                 mode,
                 GL_UNSIGNED_BYTE,
                 surface->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    tex->width = surface->w;
    tex->height = surface->h;

    SDL_FreeSurface(surface);
}

void
texbind(Texture *tex)
{
    glBindTexture(GL_TEXTURE_2D, tex->id);
}

void
texfind(Texture *tex, const char *path)
{
    int len;
    char *name = strrchr(path, '/');
    char *texname;
    Texture *texentry;

    if (!name) {
        name = (char *)path;
    }
    texentry = htableget(textures, name);

    if (texentry) {
        *tex = *texentry;
        return;
    }
    len = strlen(name);
    texentry = malloc(sizeof(Texture));
    texname = malloc(sizeof(char) * len);
    memcpy(texname, name, len);
    texload(texentry, path);
    htableadd(textures, texname, texentry);
    *tex = *texentry;
}

void
texget(Texture *tex, int x, int y, float *uvs)
{
    float texperrow = tex->imagesize / (float)tex->texsize;
    float texsize = 1.0f / texperrow;
    float pixsize = texsize / (float)tex->texsize;
    float pixoffset = pixsize * 0.001f;

    float xmin = x * texsize + pixoffset;
    float ymin = y * texsize + pixoffset;
    float xmax = (xmin + texsize) - (pixoffset * 2);
    float ymax = (ymin + texsize) - (pixoffset * 2);

    uvs[0] = xmax; uvs[1] = ymax;
    uvs[2] = xmin; uvs[3] = ymax;
    uvs[4] = xmin; uvs[5] = ymin;
    uvs[6] = xmax; uvs[7] = ymin;
}

void
texgetrect(Texture *tex, TextureRect *rect, float *uvs)
{
    float pixwidth = 1.0f / tex->width;
    float pixheight = 1.0f / tex->height;
    float pixoffset_x = pixwidth * 0.001f;
    float pixoffset_y = pixheight * 0.001f;

    float xmin = rect->x * pixwidth + pixoffset_x;
    float ymin = rect->y * pixheight + pixoffset_y;
    float xmax = (rect->x + rect->w) * pixwidth - pixoffset_x;
    float ymax = (rect->y + rect->h) * pixheight - pixoffset_y;

    uvs[0] = xmax; uvs[1] = ymax;
    uvs[2] = xmin; uvs[3] = ymax;
    uvs[4] = xmin; uvs[5] = ymin;
    uvs[6] = xmax; uvs[7] = ymin;
}

void
texsetactivetexture(uint texunit)
{
    if (texunit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
        glActiveTexture(GL_TEXTURE0 + texunit);
    } else {
        logprint(LOG_ERROR, "tex: Attempted to enable invalid texture unit");
    }
}

void
texsetsampleruniform(uint program, const char *name, uint texunit)
{
    int uniform = glGetUniformLocation(program, name);
    glUniform1i(uniform, texunit);
}

void
texquit(void)
{
    htabledelete(textures);
    //listdealloc(texturefiles);
    //listdealloc(texturedata);
    //listdelete(texturefiles);
    //listdelete(texturedata);
}

void
fontinit(void)
{
    //fontfiles = listcreate();
    //fontdata = listcreate();
    fonts = htablecreate(htablestringhash, htablestringequal);
}

void
fontbind(FontTex *font)
{
    glBindTexture(GL_TEXTURE_2D, font->id);
}

void
fontload(FontTex *font, const char *path, int width, int height)
{
    SDL_Surface *surface = IMG_Load(path);
    int mode = GL_RGB;

    if (!surface) {
        logprint(LOG_ERROR, "texture: Unable to load texture file $s: " \
                            "SDL_image error: $s", path, IMG_GetError());
        return;
    }
    glGenTextures(1, &font->id);
    glBindTexture(GL_TEXTURE_2D, font->id);

    if (surface->format->BytesPerPixel == 4) {
        mode = GL_RGBA;
    }
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 mode,
                 surface->w,
                 surface->h,
                 0,
                 mode,
                 GL_UNSIGNED_BYTE,
                 surface->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    /*glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST_MIPMAP_LINEAR);*/
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    //glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    font->width = surface->w;
    font->height = surface->h;
    font->fontwidth = width;
    font->fontheight = height;

    SDL_FreeSurface(surface);
}

void
fontfind(FontTex *font, const char *path, int width, int height)
{
    int len;
    char *name = strrchr(path, '/');
    char *fontname;
    FontTex *fontentry;

    if (!name)
        name = (char *)path;

    fontentry = htableget(fonts, name);
    
    if (fontentry) {
        *font = *fontentry;
        return;
    }
    len = strlen(name);
    fontentry = malloc(sizeof(FontTex));
    fontname = malloc(sizeof(char) * len);
    memcpy(fontname, name, len);
    fontload(fontentry, path, width, height);
    htableadd(fonts, fontname, fontentry);
    *font = *fontentry;
}

void
fontcalctexcoords(FontTex *font, uchar charcount)
{
    int i;

    for (i = 0; i < charcount; i++) {
        fontget(font, i, font->uvs[i]);
    }
}

void
fontget(FontTex *font, const char chr, float *uvs)
{
    int texperrow = font->width / font->fontwidth;
    int charpos_x = chr % texperrow;
    int charpos_y = chr / texperrow;
    int texwidth = charpos_x * font->fontwidth;
    int texheight = charpos_y * font->fontheight;
    float pixwidth = 1.0f / font->width;
    float pixheight = 1.0f / font->height;

    float xmin = texwidth * pixwidth;
    float ymin = texheight * pixheight;
    float xmax = (texwidth + font->fontwidth) * pixwidth;
    float ymax = (texheight + font->fontheight) * pixheight;

    uvs[0] = xmax; uvs[1] = ymax;
    uvs[2] = xmin; uvs[3] = ymax;
    uvs[4] = xmin; uvs[5] = ymin;
    uvs[6] = xmax; uvs[7] = ymin;
}

void
fontdelete(FontTex *font)
{
    if (font->id) {
        glDeleteTextures(1, &font->id);
    }
}

void
fontquit(void)
{
    htabledealloc(fonts);
    htabledelete(fonts);
    fonts = NULL;
}
