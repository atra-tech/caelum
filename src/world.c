/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Describes the game world, tiles, tile-bound entities and 
 *  related functions.
 *
 * */

#include "world.h"
#include "util/log.h"
#include "util/list.h"
#include "util/htable.h"
#include "agent.h"
#include "light.h"
#include "input.h"
#include "core.h"
#include "thread.h"
//#include "thpool.h"

static const float wallverts[] =
{
    0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,

    1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,

    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f,

    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f,

    0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    
    0.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 1.0f,
    1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    1.0f, -0.17f, 0.0f,
    0.0f, -0.17f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

    0.0f, -0.17f, 0.0f,
    0.0f, -0.17f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,

    1.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f,
};

static const float topfloorverts[] =
{
    0.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 1.0f,
    1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    1.0f, -0.17f, 0.0f,
    0.0f, -0.17f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

    0.0f, -0.17f, 0.0f,
    0.0f, -0.17f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,

    1.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f,

    0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
};

static const float subfloorverts[] =
{
    0.0f, -0.20f, 1.0f,
    1.0f, -0.20f, 1.0f,
    1.0f, -0.17f, 1.0f,
    0.0f, -0.17f, 1.0f,

    1.0f, -0.20f, 0.0f,
    0.0f, -0.20f, 0.0f,
    0.0f, -0.17f, 0.0f,
    1.0f, -0.17f, 0.0f,

    0.0f, -0.20f, 0.0f,
    0.0f, -0.20f, 1.0f,
    0.0f, -0.17f, 1.0f,
    0.0f, -0.17f, 0.0f,

    1.0f, -0.20f, 1.0f,
    1.0f, -0.20f, 0.0f,
    1.0f, -0.17f, 0.0f,
    1.0f, -0.17f, 1.0f,

    0.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 1.0f,
    1.0f, -0.17f, 0.0f,
    0.0f, -0.17f, 0.0f
};

static const float worldnorms[] =
{
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f,-1.0f,
   -1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f,-1.0f,
   -1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

};

enum {
    WALL_BACK,
    WALL_FRONT,
    WALL_LEFT,
    WALL_RIGHT,
    WALL_TOP,
    WALL_SUB_BACK,
    WALL_SUB_FRONT,
    WALL_SUB_LEFT,
    WALL_SUB_RIGHT
};

enum {
    TILE_TL,
    TILE_TR,
    TILE_BR,
    TILE_BL,
};

struct SegJobArgs {
    Segment *seg;
    World *world;
};

static Segment *segmentcreatematrix(uint rows, uint cols);
static Layer *layercreatearray(uint count);
static void segmentdeletematrix(Segment *segs, uint rows, uint cols);
static void layerdeletearray(Layer *layers, uint count);
//static void layerclearlight(Layer *layer);
static void lightinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsideinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);
static void lightwalltopinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsubinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);
static void spacebackgroundinit(void);
static void spacebackgroundrender(Camera *cam);
static void spacebackgrounddelete(void);
static void selectioninit(void);
static void selectionrender(Camera *cam);
static void selectiondelete(void);

static List *tiledata;
static Shader worldshader;
static TPool *workerpool;

/* Space background data */
static struct SpaceBackground {
    Shader shader;
    Texture texture;
    VBuffer vbo;
    mat4 modelmat;
} space_bg;

static struct Selection {
    Shader shader;
    VBuffer vbo;
    mat4 modelmat;
} selection;

void
worldinit()
{
    tiledatabaseinit("res/tile/manifest");
    tiledata = listcreate();
    shaderinit(&worldshader,
            "res/shader/worldvert.glsl",
            "res/shader/worldfrag.glsl");

    spacebackgroundinit();
    selectioninit();

    workerpool = tpoolcreate(SDL_GetCPUCount());
}

void
spacebackgroundinit(void)
{
    vec3 vert;
    vec2 uv;
    vec3 norm = {-1.0f, 0.0f, 0.0f};
    ivec3 idx;

    shaderinit(&space_bg.shader,
            "res/shader/spacevert.glsl",
            "res/shader/spacefrag.glsl");
    texfind(&space_bg.texture, "res/texture/space_bg.png");
    vbufinit(&space_bg.vbo);
    vbufclear(&space_bg.vbo);

    vert[0] = 0.0f;
    vert[1] = -64.0f;
    vert[2] = -64.0f;
    uv[0] = 1.0f;
    uv[1] = 1.0f;
    vbufaddvertex(&space_bg.vbo, vert, uv, norm);

    vert[0] = 0.0f;
    vert[1] = -64.0f;
    vert[2] = 64.0f;
    uv[0] = 0.0f;
    uv[1] = 1.0f;
    vbufaddvertex(&space_bg.vbo, vert, uv, norm);

    vert[0] = 0.0f;
    vert[1] = 64.0f;
    vert[2] = 64.0f;
    uv[0] = 0.0f;
    uv[1] = 0.0f;
    vbufaddvertex(&space_bg.vbo, vert, uv, norm);

    vert[0] = 0.0f;
    vert[1] = 64.0f;
    vert[2] = -64.0f;
    uv[0] = 1.0f;
    uv[1] = 0.0f;
    vbufaddvertex(&space_bg.vbo, vert, uv, norm);

    idx[0] = 0;
    idx[1] = 1;
    idx[2] = 2;
    vbufaddindex(&space_bg.vbo, idx);

    idx[0] = 2;
    idx[1] = 3;
    idx[2] = 0;
    vbufaddindex(&space_bg.vbo, idx);

    space_bg.vbo.indxcount = 12;

    vbufbind(&space_bg.vbo);
}

void
selectioninit(void)
{
    uint i, j, vertidx, normidx, indexcount;
    vec3 vert;
    vec2 uv = {0.0f, 0.0f};
    vec3 norm;
    ivec3 idx;

    shaderinit(&selection.shader,
            "res/shader/selectionvert.glsl",
            "res/shader/selectionfrag.glsl");
    vbufinit(&selection.vbo);

    indexcount = 0;

    for (i = 0; i < 4; i++) {
        normidx = i * 3;

        for (j = 0; j < 4; j++) {
            vertidx = i * 12 + j * 3;

            vert[0] = wallverts[vertidx + 0];
            vert[1] = wallverts[vertidx + 1];
            vert[2] = wallverts[vertidx + 2];

            norm[0] = worldnorms[normidx + 0];
            norm[1] = worldnorms[normidx + 1];
            norm[2] = worldnorms[normidx + 2];
            
            if (j < 2) {
                vert[1] += 0.02f;
            } else {
                vert[1] += 0.5f;
            }

            if (vert[0] > 0.0f) {
                vert[0] += 0.02f;
            } else {
                vert[0] -= 0.02f;
            }

            if (vert[2] > 0.0f) {
                vert[2] += 0.02f;
            } else {
                vert[2] -= 0.02f;
            }

            vbufaddvertex(&selection.vbo, vert, uv, norm);
        }
        idx[0] = indexcount + 0;
        idx[1] = indexcount + 1;
        idx[2] = indexcount + 2;
        vbufaddindex(&selection.vbo, idx);
        
        idx[0] = indexcount + 2;
        idx[1] = indexcount + 3;
        idx[2] = indexcount + 0;
        vbufaddindex(&selection.vbo, idx);

        indexcount += 4;
        selection.vbo.indxcount += 12;
    }
    vbufbind(&selection.vbo);
}

void
worldshutdown()
{
    listdelete(tiledata);
}

World *
worldcreate()
{
    World *world = malloc(sizeof(World));

    world->segs = segmentcreatematrix(SEGMENT_COUNT, SEGMENT_COUNT);

    // TODO: Implement world loading and saving.
    //  TEST DATA:
    uint seg_x, seg_y, tile_x, tile_y;
    Segment *seg;
    Layer *layer;
    TileData *data;

    for (seg_x = 0; seg_x < SEGMENT_COUNT; seg_x++) {
        for (seg_y = 0; seg_y < SEGMENT_COUNT; seg_y++) {
            if (seg_x < 31 || seg_x > 33) continue;
            if (seg_y < 31 || seg_y > 33) continue;

            seg = &world->segs[seg_y + seg_x * SEGMENT_COUNT];
            layer = &seg->layers[0];
            data = tilegetdata("maint_floor");

            for (tile_x = 0; tile_x < SEGMENT_SIZE; tile_x++) {
                for (tile_y = 0; tile_y < SEGMENT_SIZE; tile_y++) {
                    //layer->toptiles[tile_y + tile_x * SEGMENT_SIZE].data =
                    //    tilegetdata("res/tile/int_floor_tiled.abf");
                    layer->subtiles[tile_y + tile_x * SEGMENT_SIZE].data = 
                        data;
                }
            }
        }
    }
    return world;
}

Segment *
segmentcreatematrix(uint rows, uint cols)
{
    uint i, j, idx, segcount = rows * cols;
    Segment *segs = malloc(segcount * sizeof(Segment));

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            idx = j + i * SEGMENT_COUNT;
            segs[idx].x = i;
            segs[idx].z = j;
            segs[idx].dirty = 1;
            segs[idx].relight = 1;
            segs[idx].layers = layercreatearray(WORLD_HEIGHT);
            vbufinit(&segs[idx].vbo);
            segs[idx].vertlights = listcreate();
            segs[idx].lights = listcreate();
            glm_mat4_identity(segs[idx].modelmat);
            glm_translate_x(segs[idx].modelmat, i * SEGMENT_SIZE);
            glm_translate_z(segs[idx].modelmat, j * SEGMENT_SIZE);
        }
    }

    return segs;
}

Layer *
layercreatearray(uint count)
{
    uint i;
    Layer *layers = malloc(count * sizeof(Layer));

    for (i = 0; i < count; i++) {
        layers[i].pos = i;
        layers[i].toptiles = calloc(LAYER_TILECOUNT, sizeof(Tile));
        layers[i].subtiles = calloc(LAYER_TILECOUNT, sizeof(Tile));
        layers[i].light = calloc(LAYER_TILECOUNT, sizeof(vec3));
    }

    return layers;
}

void
worlddelete(World *world)
{
    segmentdeletematrix(world->segs, SEGMENT_COUNT, SEGMENT_COUNT);
    world->segs = NULL;
    selectiondelete();
    spacebackgrounddelete();
    free(world);
}

void
segmentdeletematrix(Segment *segs, uint rows, uint cols)
{
    uint i, j, segcount = rows * cols;

    for (i = 0; i < segcount; i++) {
        layerdeletearray(segs[i].layers, WORLD_HEIGHT);
        vbufdelete(&segs[i].vbo);
        listdelete(segs[i].vertlights);
        
        for (j = 0; j < segs[i].lights->size; j++) {
            lightdelete(listat(segs[i].lights, j));
        }
        listdelete(segs[i].lights);
    }
    free(segs);
}

void
layerdeletearray(Layer *layers, uint count)
{
    uint i;

    for (i = 0; i < count; i++) {
        free(layers[i].subtiles);
        free(layers[i].toptiles);
        free(layers[i].light);
        //listdealloc(layers->links);
        //listdelete(layers->links);
    }
    free(layers);
}

void
selectiondelete(void)
{
    shaderdelete(&selection.shader);
    vbufdelete(&selection.vbo);
}

void
spacebackgrounddelete(void)
{
    shaderdelete(&space_bg.shader);
    vbufdelete(&space_bg.vbo);
}

void
mousecalcray(Camera *cam, int mx, int my, vec3 ray)
{
    vec4 ray_clip, ray_eye;
    mat4 invproj, invview;

    ray_clip[0] = (2.0f * mx) / cam->width - 1.0f;
    ray_clip[1] = 1.0f - (2.0f * my) / cam->height;
    ray_clip[2] = -1.0f;
    ray_clip[3] = 1.0f;

    glm_mat4_inv(cam->projmat, invproj);
    glm_mat4_mulv(invproj, ray_clip, ray_eye);

    ray_eye[2] = -1.0f;
    ray_eye[3] = 0.0f;

    glm_mat4_inv(cam->viewmat, invview);
    glm_mat4_mulv3(invview, ray_eye, ray_eye[3], ray);
    glm_normalize(ray);
}

void
worldhandle(World *world, Event *e)
{
    //  Handle click events or key presses on world objects.
    //  TODO: tile picking algorithm
    int i, j;
    int mx, my, mbut, tile_x, tile_z, relight;
    vec3 ray;
    float dist;
    Camera *cam;
    Tile *tile;
    Segment *seg;

    mbut = mousegetstate(&mx, &my);
    cam = getmaincamera();
    relight = 0;

    mousecalcray(cam, mx, my, ray);
    dist = fabs(cam->ent.y / ray[1]);
    tile_x = ray[0] * dist + cam->ent.x;
    tile_z = ray[2] * dist + cam->ent.z;
    //logprint(LOG_INFO, "Got tile: x:$i, z:$i", tile_x, tile_z);

    if (tile_x < 1 || tile_x > WORLD_SIZE - 2 ||
            tile_z < 1 || tile_z > WORLD_SIZE - 2) {
        return;
    }        
    tile = worldgettoptileat(world, tile_x, 0, tile_z);
    glm_mat4_identity(selection.modelmat);    
    glm_translate_x(selection.modelmat, tile_x);
    glm_translate_z(selection.modelmat, tile_z);

    if (!tile->data) {
        glm_translate_y(selection.modelmat, -0.17);
    }
   
    //TODO: MAJOR REWORK FOR INVENTORY -> TILE PLACEMENT
    if (e->type == SDL_MOUSEMOTION) {

    } else if (e->type == SDL_MOUSEBUTTONDOWN) {
        tile = worldgettoptileat(world, tile_x, 0, tile_z);
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (tile->data) {
                if (tile->data->attr & TILE_WALL) {
                    tile->data = tilegetdata("int_floor_tiled");
                    relight = 1;
                } else {
                    tile->data = NULL;
                }
            } else {
                tile = worldgetsubtileat(world, tile_x, 0, tile_z);

                if (tile->data) {
                    tile->data = NULL;
                }
            }
        } else if (e->button.button == SDL_BUTTON_RIGHT) {
            if (tile->data) {
                if (tile->data->attr & TILE_FLOOR) {
                    tile->data = tilegetdata("int_wall_lamp");
                    relight = 1;
                }
            } else {
                Tile *top = tile;
                tile = worldgetsubtileat(world, tile_x, 0, tile_z);

                if (tile->data) {
                    top->data = tilegetdata("int_floor_tiled");
                } else {
                    tile->data = tilegetdata("maint_floor");
                }
            }
        }
        seg = worldgetsegmentat(world, tile_x, tile_z);
        seg->dirty = 1;
    } else if (e->type == SDL_MOUSEBUTTONUP) {

    }
    
    if (relight) {
        for (i = -1; i <= 1; i++) {
            for (j = -1; j <= 1; j++) {
                seg = worldgetsegmentat(world,
                        tile_x + i * SEGMENT_SIZE,
                        tile_z + j * SEGMENT_SIZE);
                seg->relight = 1;
            }
        }
    }
}

void
worldupdate(World *world, float dt)
{
    //  Update pressure and temperature values and air concentration
    //  Step pipe, wire values
    //  Call tile entities to update their states
}

void
toptilemesh(World *world, Segment *seg, uint x, uint y, uint z)
{
    uint i, j, vertidx, uvidx, normidx, indexcount, world_x, world_z;
    Tile *tile = &seg->layers->toptiles[z + x * SEGMENT_SIZE];
    VBuffer *vbo = &seg->vbo;
    vec3 vert;
    vec2 uv;
    vec3 norm;
    ivec3 index;
    float *colorvec;
    LightColor *color;
    Tile *tile_adj[4] = {NULL, NULL, NULL, NULL};

    world_x = seg->x * SEGMENT_SIZE + x;
    world_z = seg->z * SEGMENT_SIZE + z;

    if (world_z < WORLD_SIZE - 1) { 
        tile_adj[0] = worldgettoptileat(world, world_x, y, world_z + 1);
    }
    if (world_z > 0) {
        tile_adj[1] = worldgettoptileat(world, world_x, y, world_z - 1);
    }
    if (world_x > 0) { 
        tile_adj[2] = worldgettoptileat(world, world_x - 1, y, world_z);
    }
    if (world_x < WORLD_SIZE - 1) {
        tile_adj[3] = worldgettoptileat(world, world_x + 1, y, world_z);
    }

    indexcount = vbo->indxcount / 3;
    color = &seg->layers[y].light[z + x * SEGMENT_SIZE];

    if (tile->data->attr & TILE_WALL) {
        for (i = 0; i < 9; i++) {
            normidx = i * 3;

            /* Cull faces */
            switch (i) {
            case WALL_BACK:
                if (tile_adj[0] && tile_adj[0]->data)
                    if (tile_adj[0]->data->attr & TILE_WALL)
                        continue;
                break;
            case WALL_FRONT:
                if (tile_adj[1] && tile_adj[1]->data)
                    if (tile_adj[1]->data->attr & TILE_WALL)
                        continue;
                break;
            case WALL_LEFT:
                if (tile_adj[2] && tile_adj[2]->data)
                    if (tile_adj[2]->data->attr & TILE_WALL)
                        continue;
                break;
            case WALL_RIGHT:
                if (tile_adj[3] && tile_adj[3]->data)
                    if (tile_adj[3]->data->attr & TILE_WALL)
                        continue;
            }
            
            switch (i - 5) {
            case WALL_BACK:
                if (tile_adj[0] && tile_adj[0]->data)
                    continue;
                break;
            case WALL_FRONT:
                if (tile_adj[1] && tile_adj[1]->data)
                    continue;
                break;
            case WALL_LEFT:
                if (tile_adj[2] && tile_adj[2]->data)
                    continue;
                break;
            case WALL_RIGHT:
                if (tile_adj[3] && tile_adj[3]->data)
                    continue;
            }

            for (j = 0; j < 4; j++) {
                vertidx = i * 12 + j * 3;
                uvidx = j * 2;
                vert[0] = wallverts[vertidx + 0] + x;
                vert[1] = wallverts[vertidx + 1] + y;
                vert[2] = wallverts[vertidx + 2] + z;

                colorvec = malloc(3 * sizeof(float));
                colorvec[0] = 0.0f;
                colorvec[1] = 0.0f;
                colorvec[2] = 0.0f;

                if (i < 4) {
                    lightwallsideinterpolate(world, world_x, y, world_z,
                            colorvec, i, j);
                } else if (i < 5) {
                    lightwalltopinterpolate(world, world_x, y, world_z,
                            colorvec, j);
                } else {
                    lightwallsubinterpolate(world, world_x, y, world_z,
                            colorvec, i, j);
                }
                listpushback(seg->vertlights, colorvec);

                if (i < 4) {
                    uv[0] = tile->data->sidetex[uvidx + 0];
                    uv[1] = tile->data->sidetex[uvidx + 1];
                } else if (i < 5) {
                    uv[0] = tile->data->toptex[uvidx + 0];
                    uv[1] = tile->data->toptex[uvidx + 1];
                } else {
                    uv[0] = tile->data->subsidetex[uvidx + 0];
                    uv[1] = tile->data->subsidetex[uvidx + 1];
                }
                norm[0] = worldnorms[normidx + 0];
                norm[1] = worldnorms[normidx + 1];
                norm[2] = worldnorms[normidx + 2];

                vbufaddvertex(vbo, vert, uv, norm);
            }
            index[0] = indexcount + 0;
            index[1] = indexcount + 1;
            index[2] = indexcount + 2;
            vbufaddindex(vbo, index);
            
            index[0] = indexcount + 2;
            index[1] = indexcount + 3;
            index[2] = indexcount + 0;
            vbufaddindex(vbo, index);

            indexcount += 4;
            vbo->indxcount += 12;
        }
    } else {
        for (i = 0; i < 5; i++) {
            normidx = i * 3;

            switch (i) {
            case WALL_BACK:
                if (tile_adj[0] && tile_adj[0]->data) {
                    if (tile_adj[0]->data->attr & TILE_WALL) {
                        continue;
                    }
                }
                break;
            case WALL_FRONT:
                if (tile_adj[1] && tile_adj[1]->data) {
                    if (tile_adj[1]->data->attr & TILE_WALL) {
                        continue;
                    }
                }
                break;
            case WALL_LEFT:
                if (tile_adj[2] && tile_adj[2]->data) {
                    if (tile_adj[2]->data->attr & TILE_WALL) {
                        continue;
                    }
                }
                break;
            case WALL_RIGHT:
                if (tile_adj[3] && tile_adj[3]->data) {
                    if (tile_adj[3]->data->attr & TILE_WALL) {
                        continue;
                    }
                }
            }
            
            for (j = 0; j < 4; j++) {
                vertidx = i * 12 + j * 3;
                uvidx = j * 2;
                vert[0] = topfloorverts[vertidx + 0] + x;
                vert[1] = topfloorverts[vertidx + 1] + y;
                vert[2] = topfloorverts[vertidx + 2] + z;

                if (i < 4) {
                    colorvec = malloc(3 * sizeof(float));
                    colorvec[0] = color->r * 0.5;
                    colorvec[1] = color->g * 0.5;
                    colorvec[2] = color->b * 0.5;
                    listpushback(seg->vertlights, colorvec);

                    uv[0] = tile->data->sidetex[uvidx + 0];
                    uv[1] = tile->data->sidetex[uvidx + 1];
                } else {
                    colorvec = malloc(3 * sizeof(float));
                    colorvec[0] = color->r;
                    colorvec[1] = color->g;
                    colorvec[2] = color->b;
                    lightinterpolate(world, world_x, y, world_z, colorvec, j);
                    listpushback(seg->vertlights, colorvec);

                    uv[0] = tile->data->toptex[uvidx + 0];
                    uv[1] = tile->data->toptex[uvidx + 1];
                }
                norm[0] = worldnorms[normidx + 0];
                norm[1] = worldnorms[normidx + 1];
                norm[2] = worldnorms[normidx + 2];

                vbufaddvertex(vbo, vert, uv, norm);
            }
            index[0] = indexcount + 0;
            index[1] = indexcount + 1;
            index[2] = indexcount + 2;
            vbufaddindex(vbo, index);

            index[0] = indexcount + 2;
            index[1] = indexcount + 3;
            index[2] = indexcount + 0;
            vbufaddindex(vbo, index);

            indexcount += 4;
            vbo->indxcount += 12;
        }
    }
}

void
subtilemesh(World *world, Segment *seg, uint x, uint y, uint z)
{
    uint i, j, vertidx, uvidx, normidx, indexcount;
    Tile *tile = &seg->layers[y].subtiles[z + x * SEGMENT_SIZE];
    VBuffer *vbo = &seg->vbo;
    vec3 vert;
    vec2 uv;
    vec3 norm;
    ivec3 index;
    float *colorvec;
    LightColor *color;

    indexcount = vbo->indxcount / 3;
    color = &seg->layers[y].light[z + x * SEGMENT_SIZE];

    for (i = 0; i < 5; i++) {
        normidx = i * 3;

        for (j = 0; j < 4; j++) {
            vertidx = i * 12 + j * 3;
            uvidx = j * 2;
            vert[0] = subfloorverts[vertidx + 0] + x;
            vert[1] = subfloorverts[vertidx + 1] + y;
            vert[2] = subfloorverts[vertidx + 2] + z;

            if (i < 4) {
                colorvec = malloc(3 * sizeof(float));
                colorvec[0] = color->r * 0.5;
                colorvec[1] = color->g * 0.5;
                colorvec[2] = color->b * 0.5;
                listpushback(seg->vertlights, colorvec);

                uv[0] = tile->data->sidetex[uvidx + 0];
                uv[1] = tile->data->sidetex[uvidx + 1];
            } else {
                colorvec = malloc(3 * sizeof(float));
                colorvec[0] = color->r;
                colorvec[1] = color->g;
                colorvec[2] = color->b;
                lightinterpolate(world, seg->x * SEGMENT_SIZE + x, y,
                        seg->z * SEGMENT_SIZE + z, colorvec, j);
                listpushback(seg->vertlights, colorvec);

                uv[0] = tile->data->toptex[uvidx + 0];
                uv[1] = tile->data->toptex[uvidx + 1];
            }
            norm[0] = worldnorms[normidx + 0];
            norm[1] = worldnorms[normidx + 1];
            norm[2] = worldnorms[normidx + 2];

            vbufaddvertex(vbo, vert, uv, norm);
        }
        index[0] = indexcount + 0;
        index[1] = indexcount + 1;
        index[2] = indexcount + 2;
        vbufaddindex(vbo, index);

        index[0] = indexcount + 2;
        index[1] = indexcount + 3;
        index[2] = indexcount + 0;
        vbufaddindex(vbo, index);

        indexcount += 4;
        vbo->indxcount += 12;
    }
}

void
segmentmesh(World *world, Segment *seg)
{
    uint x, y, z;
    Layer *layer;
    Tile *tile;

    //TODO:  Only mesh on player's y-level and below
    for (y = 0; y < WORLD_HEIGHT; y++) {
        layer = &seg->layers[y];

        for (x = 0; x < SEGMENT_SIZE; x++) {
            for (z = 0; z < SEGMENT_SIZE; z++) {
                tile = &layer->toptiles[z + x * SEGMENT_SIZE];

                if (tile->data) {
                    toptilemesh(world, seg, x, y, z);
                } else {
                    tile = &layer->subtiles[z + x * SEGMENT_SIZE];
                    if (tile->data) {
                        subtilemesh(world, seg, x, y, z);
                    }
                }
            }
        }
    }
/*    logprint(LOG_INFO, "Meshed segment: x:$i, z:$i", seg->x, seg->z);
*/
}

void
segmentlightjob(void *arg)
{
    struct SegJobArgs *args;
    args = arg;

    Segment *seg = args->seg;
    World *world = args->world;

    segmentrelight(world, seg);
    seg->relight = 0;
    seg->dirty = 1;

    free(args);
}

void
segmentmeshjob(void *arg)
{
    struct SegJobArgs *args;
    args = arg;

    Segment *seg = args->seg;
    World *world = args->world;

    segmentmesh(world, seg);

    printf("[Thread: %lu] Meshed segment: x=%d, z=%d\n",
            SDL_ThreadID(), seg->x, seg->z);
    
    free(args);
}

void segmentqueueremesh(World *world, Segment *seg)
{
    struct SegJobArgs *args; 
    
    args = malloc(sizeof(struct SegJobArgs));
    args->seg = seg;
    args->world = world;

    tpooladdjob(workerpool, segmentmeshjob, args);
}

void segmentqueuerelight(World *world, Segment *seg)
{
    struct SegJobArgs *args; 
    
    args = malloc(sizeof(struct SegJobArgs));
    args->seg = seg;
    args->world = world;

    tpooladdjob(workerpool, segmentlightjob, args);
}

void
worldrender(World *world, Camera *cam)
{
    Agent *player = agentgetplayer();
    Segment *seg;
    uint pseg_x, pseg_z;
    int i, j, seg_x, seg_z;

    pseg_x = player->ent.x / SEGMENT_SIZE;
    pseg_z = player->ent.z / SEGMENT_SIZE;

    spacebackgroundrender(cam);

    texsetactivetexture(0);
    texbind(tilegetalbedotex());
    texsetactivetexture(1);
    texbind(tilegetemissiontex());
    //texsetactivetexture(2);
    //texbind(tilegetnormaltex());

    shaderbind(&worldshader);
    shaderloadprojviewmat(&worldshader, cam->projviewmat);
    shaderloadviewmat(&worldshader, cam->viewmat);

    shaderaddint(&worldshader, "albedo_map", 0);
    shaderaddint(&worldshader, "emission_map", 1);
    //shaderaddint(&worldshader, "normal_map", 2);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    //TODO: Split surrounding segment checks into single loop with list
    for (i = -2; i <= 2; i++) {
        seg_x = pseg_x + i;

        if (seg_x < 0 || seg_x >= SEGMENT_COUNT) {
            continue;
        }

        for (j = -2; j <= 2; j++) {
            seg_z = pseg_z + j;
            seg = &world->segs[seg_z + seg_x * SEGMENT_COUNT];
            
            if (seg_z < 0 || seg_z >= SEGMENT_COUNT) {
                continue;
            }

            if (seg->relight || seg->dirty) {
                vbufreset(&seg->vbo);
                listdealloc(seg->vertlights);
                listclear(seg->vertlights);
            }
        }
    }
    for (i = -2; i <= 2; i++) {
        seg_x = pseg_x + i;

        if (seg_x < 0 || seg_x >= SEGMENT_COUNT) {
            continue;
        }

        for (j = -2; j <= 2; j++) {
            seg_z = pseg_z + j;
            seg = &world->segs[seg_z + seg_x * SEGMENT_COUNT];
            
            if (seg_z < 0 || seg_z >= SEGMENT_COUNT) {
                continue;
            }

            if (seg->relight) {
                segmentqueuerelight(world, seg);
            }
        }
    }
    tpoolwait(workerpool);

    for (i = -2; i <= 2; i++) {
        seg_x = pseg_x + i;

        if (seg_x < 0 || seg_x >= SEGMENT_COUNT) {
            continue;
        }

        for (j = -2; j <= 2; j++) {
            seg_z = pseg_z + j;
            seg = &world->segs[seg_z + seg_x * SEGMENT_COUNT];
            
            if (seg_z < 0 || seg_z >= SEGMENT_COUNT) {
                continue;
            }

            if (seg->dirty) {
                segmentqueueremesh(world, seg);
            }
        }
    }
    tpoolwait(workerpool);

    for (i = -2; i <= 2; i++) {
        seg_x = pseg_x + i;

        if (seg_x < 0 || seg_x >= SEGMENT_COUNT) {
            continue;
        }

        for (j = -2; j <= 2; j++) {
            seg_z = pseg_z + j;
            seg = &world->segs[seg_z + seg_x * SEGMENT_COUNT];
            
            if (seg_z < 0 || seg_z >= SEGMENT_COUNT) {
                continue;
            }

            if (seg->dirty) {
                vbufbind(&seg->vbo);
                vbufbindvbo(&seg->vbo, 3, seg->vertlights);
                seg->dirty = 0;
            }

            glBindVertexArray(seg->vbo.vao);
            shaderloadmodelmat(&worldshader, seg->modelmat);
            glDrawElements(GL_TRIANGLES, seg->vbo.indxcount,
                    GL_UNSIGNED_INT, NULL);
        }
    }
    shaderunbind();
    selectionrender(cam);
}

void
spacebackgroundrender(Camera *cam)
{
    Agent *player = agentgetplayer();

    glm_mat4_identity(space_bg.modelmat);
    glm_translate_x(space_bg.modelmat, player->ent.x);
    glm_translate_y(space_bg.modelmat, -10.0f);
    glm_translate_z(space_bg.modelmat, player->ent.z);
    glm_rotate_z(space_bg.modelmat, 5.06f, space_bg.modelmat);

    texsetactivetexture(0);
    texbind(&space_bg.texture);

    shaderbind(&space_bg.shader);
    shaderloadprojviewmat(&space_bg.shader, cam->projviewmat);
    shaderloadviewmat(&space_bg.shader, cam->viewmat);
    shaderloadmodelmat(&space_bg.shader, space_bg.modelmat);
    shaderaddint(&space_bg.shader, "spacetex", 0);
    
    glBindVertexArray(space_bg.vbo.vao);
    glDrawElements(GL_TRIANGLES, space_bg.vbo.indxcount,
            GL_UNSIGNED_INT, NULL);
    shaderunbind();
}

void
selectionrender(Camera *cam)
{
    shaderbind(&selection.shader);
    shaderloadprojviewmat(&selection.shader, cam->projviewmat);
    shaderloadviewmat(&selection.shader, cam->viewmat);
    shaderloadmodelmat(&selection.shader, selection.modelmat);

    glBindVertexArray(selection.vbo.vao);
    glDrawElements(GL_LINES, selection.vbo.indxcount,
            GL_UNSIGNED_INT, NULL);
    shaderunbind();
}

void
worldaddlight(World *world, uint x, uint y, uint z,
        float r, float g, float b, uint radius)
{
    int i, j;
    Light *light;
    Segment *seg;

    light = lightcreate(x, y, z, radius);
    lightsetcolor(light, r, g, b);
    seg = worldgetsegmentat(world, x, z);
    listpushback(seg->lights, light);

    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            seg = worldgetsegmentat(world,
                    x + i * SEGMENT_SIZE,
                    z + j * SEGMENT_SIZE);
            seg->relight = 1;
        }
    }
}

void
worldremovelight(World *world, uint x, uint y, uint z)
{
    uint i;
    Segment *seg;
    Light *light;

    seg = worldgetsegmentat(world, x, z);

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);

        if ((uint)light->x == x &&
                (uint)light->y == y &&
                (uint)light->z == z) {
            lightremovelight(light, world);
            listremove(seg->lights, i);
            lightdelete(light);
        }
    }
}

void
segmentrelight(World *world, Segment *seg)
{
    uint i;
    Light *light;

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);
        lightremovelight(light, world);
        lightcalclightmap(light, world);
        lightapplylight(light, world);
    }
}

void
lightinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner)
{
    uint i;
    LightColor *color_adj[3];

    if (x < 1) {
        x = 1;
    } else if (x >= WORLD_SIZE - 1) {
        x = WORLD_SIZE - 2;
    }

    if (z < 1) {
        z = 1;
    } else if (z >= WORLD_SIZE - 1) {
        z = WORLD_SIZE - 2;
    }

    switch (corner) {
    case TILE_BR:
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z - 1);
        break;
    case TILE_BL:
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z - 1);
        break;
    case TILE_TL:
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z + 1);
        break;
    case TILE_TR:
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z + 1);
    }

    for (i = 0; i < 3; i++) {
        color[0] += color_adj[i]->r;
        color[1] += color_adj[i]->g;
        color[2] += color_adj[i]->b;
    }

    color[0] *= 0.25;
    color[1] *= 0.25;
    color[2] *= 0.25;
}

void
lightwallsideinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner)
{
    float mult, count;
    Tile *tile_adj;
    LightColor *color_adj[2];

    count = 2;

    if (x < 1) {
        x = 1;
    } else if (x >= WORLD_SIZE - 1) {
        x = WORLD_SIZE - 2;
    }

    if (z < 1) {
        z = 1;
    } else if (z >= WORLD_SIZE - 1) {
        z = WORLD_SIZE - 2;
    }

    switch (side) {
    case WALL_FRONT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x + 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        } else {
            tile_adj = worldgettoptileat(world, x - 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        }
        color_adj[0] = worldgetlightcellat(world, x, y, z - 1);
        break;
    case WALL_BACK:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x - 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        } else {
            tile_adj = worldgettoptileat(world, x + 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        }
        color_adj[0] = worldgetlightcellat(world, x, y, z + 1);
        break;
    case WALL_LEFT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x - 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        } else {
            tile_adj = worldgettoptileat(world, x - 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        }
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z);
        break;
    case WALL_RIGHT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x + 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        } else {
            tile_adj = worldgettoptileat(world, x + 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        }
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z);
        break;
    }

    if (corner < 2) {
        mult = 0.8f;
    } else {
        mult = 0.4f;
    }

    if (tile_adj && tile_adj->data) {
        if (tile_adj->data->attr & TILE_FLOOR) {
            color[0] += color_adj[1]->r * mult;
            color[1] += color_adj[1]->g * mult;
            color[2] += color_adj[1]->b * mult;
            count--;
        }
    } else {
        color[0] += color_adj[1]->r * mult;
        color[1] += color_adj[1]->g * mult;
        color[2] += color_adj[1]->b * mult;
        count--;
    }

    color[0] += color_adj[0]->r * mult;
    color[1] += color_adj[0]->g * mult;
    color[2] += color_adj[0]->b * mult;

    if (count > 1) {
        color[0] *= 0.5f;
        color[1] *= 0.5f;
        color[2] *= 0.5f;
    }
}

void 
lightwalltopinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner)
{
    uint i;
    LightColor *color_adj[3];

    if (x < 1) {
        x = 1;
    } else if (x >= WORLD_SIZE - 1) {
        x = WORLD_SIZE - 2;
    }

    if (z < 1) {
        z = 1;
    } else if (z >= WORLD_SIZE - 1) {
        z = WORLD_SIZE - 2;
    }

    switch (corner) {
    case TILE_BR:
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z - 1);
        break;
    case TILE_BL:
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z - 1);
        break;
    case TILE_TL:
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z + 1);
        break;
    case TILE_TR:
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z + 0);
        color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        color_adj[2] = worldgetlightcellat(world, x + 0, y, z + 1);
    }

    for (i = 0; i < 3; i++) {
        color[0] += color_adj[i]->r;
        color[1] += color_adj[i]->g;
        color[2] += color_adj[i]->b;
    }

    color[0] *= 0.225;
    color[1] *= 0.225;
    color[2] *= 0.225;
}

void
lightwallsubinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner)
{
    float count;
    Tile *tile_adj;
    LightColor *color_adj[2];

    side -= 5;
    count = 2;

    if (x < 1) {
        x = 1;
    } else if (x >= WORLD_SIZE - 1) {
        x = WORLD_SIZE - 2;
    }

    if (z < 1) {
        z = 1;
    } else if (z >= WORLD_SIZE - 1) {
        z = WORLD_SIZE - 2;
    }

    switch (side) {
    case WALL_FRONT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x + 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        } else {
            tile_adj = worldgettoptileat(world, x - 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        }
        color_adj[0] = worldgetlightcellat(world, x, y, z - 1);
        break;
    case WALL_BACK:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x - 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        } else {
            tile_adj = worldgettoptileat(world, x + 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        }
        color_adj[0] = worldgetlightcellat(world, x, y, z + 1);
        break;
    case WALL_LEFT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x - 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z - 1);
        } else {
            tile_adj = worldgettoptileat(world, x - 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x - 1, y, z + 1);
        }
        color_adj[0] = worldgetlightcellat(world, x - 1, y, z);
        break;
    case WALL_RIGHT:
        if (corner == TILE_BL || corner == TILE_TL) {
            tile_adj = worldgettoptileat(world, x + 1, y, z + 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z + 1);
        } else {
            tile_adj = worldgettoptileat(world, x + 1, y, z - 1);
            color_adj[1] = worldgetlightcellat(world, x + 1, y, z - 1);
        }
        color_adj[0] = worldgetlightcellat(world, x + 1, y, z);
        break;
    }

    if (tile_adj && tile_adj->data) {
        if (tile_adj->data->attr & TILE_FLOOR) {
            color[0] += color_adj[1]->r * 0.8f;
            color[1] += color_adj[1]->g * 0.8f;
            color[2] += color_adj[1]->b * 0.8f;
            count--;
        }
    } else {
        color[0] += color_adj[1]->r * 0.8f;
        color[1] += color_adj[1]->g * 0.8f;
        color[2] += color_adj[1]->b * 0.8f;
        count--;
    }

    color[0] += color_adj[0]->r * 0.8f;
    color[1] += color_adj[0]->g * 0.8f;
    color[2] += color_adj[0]->b * 0.8f;

    if (count > 1) {
        color[0] *= 0.25f;
        color[1] *= 0.25f;
        color[2] *= 0.25f;
    } else {
        color[0] *= 0.5f;
        color[1] *= 0.5f;
        color[2] *= 0.5f;
    }
}

Segment *
worldgetsegmentat(World *world, uint x, uint z)
{
    x /= SEGMENT_SIZE;
    z /= SEGMENT_SIZE;

    return &world->segs[z + x * SEGMENT_COUNT];
}

Layer *
worldgetlayerat(World *world, uint x, uint y, uint z)
{
    x /= SEGMENT_SIZE;
    z /= SEGMENT_SIZE;

    return &world->segs[z + x * SEGMENT_COUNT].layers[y];
}

LightColor *
worldgetlightcellat(World *world, uint x, uint y, uint z)
{
    Layer *layer = worldgetlayerat(world, x, y, z);

    x %= SEGMENT_SIZE;
    z %= SEGMENT_SIZE;

    return &layer->light[z + x * SEGMENT_SIZE];
}

Tile *
worldgettoptileat(World *world, uint x, uint y, uint z)
{
    Layer *layer = worldgetlayerat(world, x, y, z);

    x %= SEGMENT_SIZE;
    z %= SEGMENT_SIZE;

    return &layer->toptiles[z + x * SEGMENT_SIZE];
}

Tile *
worldgetsubtileat(World *world, uint x, uint y, uint z)
{
    Layer *layer = worldgetlayerat(world, x, y, z);

    x %= SEGMENT_SIZE;
    z %= SEGMENT_SIZE;

    return &layer->subtiles[z + x * SEGMENT_SIZE];
}

Tile *
segmentgettoptile(Segment *seg, uint x, uint y, uint z)
{
    return &seg->layers[y].toptiles[z + x * SEGMENT_SIZE];
}

Tile *
segmentgetsubtile(Segment *seg, uint x, uint y, uint z)
{
    return &seg->layers[y].subtiles[z + x * SEGMENT_SIZE];
}
