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
#include "input.h"
#include "core.h"
#include "thread.h"

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
/*
static void lightinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsideinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);
static void lightwalltopinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsubinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);
        */
static void spacebackgroundinit(void);
static void spacebackgroundrender(Camera *cam);
static void spacebackgrounddelete(void);
static void selectioninit(void);
static void selectionrender(Camera *cam);
static void selectiondelete(void);

static List *tiledata;
static Shader worldshader;
static TPool *workerpool;
static int prev_seg_x, prev_seg_z;

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

    space_bg.vbo.indxcount = 6;

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
        selection.vbo.indxcount += 6;
    }
    vbufbind(&selection.vbo);
}

void
worldshutdown()
{
    tiledatabasestop();
    listdelete(tiledata);
    tiledata = NULL;
}

World *
worldcreate()
{
    World *world = malloc(sizeof(World));

    world->segs = segmentcreatematrix(SEGMENT_COUNT, SEGMENT_COUNT);

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
                    layer->subtiles[tile_y + tile_x * SEGMENT_SIZE].data = data;
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

            segs[idx].light_tex.width = 0;
            segs[idx].light_tex.height = 0;
            segs[idx].light_tex.id = 0;
            
            fastvboinit(&segs[idx].vbo);
            veclistinit(&segs[idx].tangents, 3);
            veclistinit(&segs[idx].bitangents, 3);

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
    uint i, vertcount;
    Layer *layers = malloc(count * sizeof(Layer));

    vertcount = (SEGMENT_SIZE + 1);
    vertcount *= vertcount;

    for (i = 0; i < count; i++) {
        layers[i].pos = i;
        layers[i].toptiles = calloc(LAYER_TILECOUNT, sizeof(Tile));
        layers[i].subtiles = calloc(LAYER_TILECOUNT, sizeof(Tile));
    }

    return layers;
}

void
worlddelete(World *world)
{
    segmentdeletematrix(world->segs, SEGMENT_COUNT, SEGMENT_COUNT);
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
        fastvbodelete(&segs[i].vbo);
        veclistdelete(&segs[i].tangents);
        veclistdelete(&segs[i].bitangents);
        lighttex_destroy(&segs[i].light_tex);
        
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
    int mx, my, mbut, tile_x, tile_z, relight, dirty;
    uint seg_x, seg_z;
    vec3 ray;
    float dist;
    Camera *cam;
    Tile *tile;
    Segment *seg;

    mbut = mousegetstate(&mx, &my);
    cam = getmaincamera();
    relight = 0;
    dirty = 0;

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
        dirty = 1;
    } else if (e->type == SDL_MOUSEBUTTONUP) {

    }

    for (i = -2; i <= 2; i++) {
        for (j = -2; j <= 2; j++) {
            seg_x = tile_x + i * SEGMENT_SIZE;
            seg_z = tile_z + j * SEGMENT_SIZE;

            if (seg_x > WORLD_SIZE - 1) continue;
            if (seg_z > WORLD_SIZE - 1) continue;

            seg = worldgetsegmentat(world, seg_x, seg_z);
            if (relight) {
                seg->relight = 1;
            } 
            if (dirty) {
                seg->dirty = 1;
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
    static int init = 0;

    Camera *cam;
    Entity *player;
    int seg_x, seg_z;
    int i, j, k;
    Segment *segs[VIEW_DIST_AREA];

    cam = getmaincamera();    
    player = cam->boundent;

    seg_x = player->x / SEGMENT_SIZE;
    seg_z = player->z / SEGMENT_SIZE;

    const int half_dist = VIEW_DIST / 2;

    if (!init) {
        /* Initialise previous segments */
        prev_seg_x = seg_x;
        prev_seg_z = seg_z;
        init = 1;

        for (i = -half_dist; i <= half_dist; i++) {
            for (j = -half_dist; j <= half_dist; j++) {
                segs[0] = worldgetsegmentat(world, player->x + i * SEGMENT_SIZE, player->z + j * SEGMENT_SIZE);
                segs[0]->light_tex = lighttex_create(2048, 2048);
                segmentflagfullrelight(world, segs[0]);
            }
        }
        return;
    }

    if (seg_x == prev_seg_x && seg_z == prev_seg_z) {
        return;
    }
    k = 0;

    /* Gather surrounding segments on position change */
    for (i = -half_dist; i <= half_dist; i++) {
        for (j = -half_dist; j <= half_dist; j++) {
            segs[k] = worldgetsegmentat(world, prev_seg_x + i, prev_seg_z + j);
            k++;
        }
    }

    /* Shift and update lightmaps based on change in player segment */
    if (seg_x == prev_seg_x + 1) {
        /* Shift lightmaps in -X direction */
        for (i = 0; i < VIEW_DIST; i++) {
            const uint tex = segs[i * VIEW_DIST]->light_tex.id;

            for (j = 0; j < VIEW_DIST - 1; j++) {
                const uint index = i * VIEW_DIST + j;
                segs[index]->light_tex.id = segs[index + 1]->light_tex.id;
            }
            segs[i * VIEW_DIST + VIEW_DIST - 1]->light_tex.id = tex;
            segs[i * VIEW_DIST + VIEW_DIST - 1]->relight = 1;
        }
    } else if (seg_x == prev_seg_x - 1) {
        /* Shift lightmaps in +X direction */
        for (i = 0; i < VIEW_DIST; i++) {
            const uint tex = segs[i * VIEW_DIST + VIEW_DIST - 1]->light_tex.id;

            for (j = 1; j < VIEW_DIST; j++) {
                const uint index = i * VIEW_DIST + j;
                segs[index]->light_tex.id = segs[index - 1]->light_tex.id;
            }
            segs[i * VIEW_DIST]->light_tex.id = tex;
            segs[i * VIEW_DIST]->relight = 1;
        }
    } else if (seg_x != prev_seg_x) {
        /* Too far away! Recalculate all lights */
        for (i = 0; i < VIEW_DIST_AREA; i++) {
            segmentflagfullrelight(world, segs[i]);
        }
    }

    if (seg_z == prev_seg_z + 1) {
        /* Shift lightmaps in -Z direction */
        for (i = 0; i < VIEW_DIST; i++) {
            const uint tex = segs[i]->light_tex.id;

            for (j = 0; j < VIEW_DIST - 1; j++) {
                const uint index = j * VIEW_DIST + i;
                segs[index]->light_tex.id = segs[index + VIEW_DIST]->light_tex.id;
            }
            segs[(VIEW_DIST - 1) * VIEW_DIST + i]->light_tex.id = tex;
            segs[(VIEW_DIST - 1) * VIEW_DIST + i]->relight = 1;
        }
    } else if (seg_z == prev_seg_z - 1) {
        /* Shift lightmaps in +Z direction */
        for (i = 0; i < VIEW_DIST; i++) {
            const uint tex = segs[(VIEW_DIST - 1) * VIEW_DIST + i]->light_tex.id;

            for (j = 1; j < VIEW_DIST; j++) {
                const uint index = j * VIEW_DIST + i;
                segs[index]->light_tex.id = segs[index - VIEW_DIST]->light_tex.id;
            }
            segs[i]->light_tex.id = tex;
            segs[i]->relight = 1;
        }
    } else {
        /* Too far away! Relight all segments */
        for (i = 0; i < VIEW_DIST_AREA; i++) {
            segmentflagfullrelight(world, segs[i]);
        }
    }

    prev_seg_x = seg_x;
    prev_seg_z = seg_z;
}

void
toptilemesh(World *world, Segment *seg, uint x, uint y, uint z)
{
    uint i, j, vertidx, uvidx, normidx, indexcount, world_x, world_z;
    Layer *layer = seg->layers + y;
    Tile *tile = layer->toptiles + z + x * SEGMENT_SIZE;
    FastVBO *vbo = &seg->vbo;
    vec3 vert, norm;
    vec2 uv;
    ivec3 index;
    Tile *tile_adj[4] = {NULL, NULL, NULL, NULL};
    
    vec3 verts[4];
    vec3 uvs[4];
    vec3 bitangent, tangent;
    vec3 edge1, edge2;
    vec2 delta_uv1, delta_uv2;
    float f;


    world_x = seg->x * SEGMENT_SIZE + x;
    world_z = seg->z * SEGMENT_SIZE + z;

    if (world_z < WORLD_SIZE - 1) { 
        tile_adj[0] = worldgettoptileat(world, world_x, y, world_z + 1);
    }
    tile_adj[1] = worldgettoptileat(world, world_x, y, world_z - 1);
    tile_adj[2] = worldgettoptileat(world, world_x - 1, y, world_z);

    if (world_x < WORLD_SIZE - 1) {
        tile_adj[3] = worldgettoptileat(world, world_x + 1, y, world_z);
    }

    indexcount = (vbo->indxcount * 2) / 3;

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
                memcpy(norm, worldnorms + normidx, 3 * sizeof(float));
                memcpy(verts[j], vert, 3 * sizeof(float));
                memcpy(uvs[j], uv, 2 * sizeof(float));
                fastvboaddvertex(vbo, vert, uv, norm);
            }
            /* Tangent and Bi-tangent calculation */
            glm_vec3_sub(verts[1], verts[0], edge1);
            glm_vec3_sub(verts[2], verts[0], edge2);

            glm_vec2_sub(uvs[1], uvs[0], delta_uv1);
            glm_vec2_sub(uvs[2], uvs[0], delta_uv2);

            f = 1.0f / (delta_uv1[0] * delta_uv2[1] - delta_uv2[0] * delta_uv1[1]);
       
            tangent[0] = f * (delta_uv2[1] * edge1[0] - delta_uv1[1] * edge2[0]);
            tangent[1] = f * (delta_uv2[1] * edge1[1] - delta_uv1[1] * edge2[1]);
            tangent[2] = f * (delta_uv2[1] * edge1[2] - delta_uv1[1] * edge2[2]);

            bitangent[0] = f * (-delta_uv2[0] * edge1[0] + delta_uv1[0] * edge2[0]);
            bitangent[1] = f * (-delta_uv2[0] * edge1[1] + delta_uv1[0] * edge2[1]);
            bitangent[2] = f * (-delta_uv2[0] * edge1[2] + delta_uv1[0] * edge2[2]);

            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);

            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);

            /* Index calculation */
            index[0] = indexcount + 0;
            index[1] = indexcount + 1;
            index[2] = indexcount + 2;
            fastvboaddindex(vbo, index);
            
            index[0] = indexcount + 2;
            index[1] = indexcount + 3;
            index[2] = indexcount + 0;
            fastvboaddindex(vbo, index);

            indexcount += 4;
            vbo->indxcount += 6;
        }
    } else {
        for (i = 0; i < 5; i++) {
            normidx = i * 3;

            switch (i) {
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
                vert[0] = topfloorverts[vertidx + 0] + x;
                vert[1] = topfloorverts[vertidx + 1] + y;
                vert[2] = topfloorverts[vertidx + 2] + z;

                if (i < 4) {
                    memcpy(uv, tile->data->sidetex + uvidx, 2 * sizeof(float));
                } else {
                    memcpy(uv, tile->data->toptex + uvidx, 2 * sizeof(float));
                }
                memcpy(norm, worldnorms + normidx, 3 * sizeof(float));
                memcpy(verts[j], vert, 3 * sizeof(float));
                memcpy(uvs[j], uv, 2 * sizeof(float));
                fastvboaddvertex(vbo, vert, uv, norm);
            }
            /* Tangent and Bi-tangent calculation */
            glm_vec3_sub(verts[1], verts[0], edge1);
            glm_vec3_sub(verts[2], verts[0], edge2);

            glm_vec2_sub(uvs[1], uvs[0], delta_uv1);
            glm_vec2_sub(uvs[2], uvs[0], delta_uv2);

            f = 1.0f / (delta_uv1[0] * delta_uv2[1] - delta_uv2[0] * delta_uv1[1]);
       
            tangent[0] = f * (delta_uv2[1] * edge1[0] - delta_uv1[1] * edge2[0]);
            tangent[1] = f * (delta_uv2[1] * edge1[1] - delta_uv1[1] * edge2[1]);
            tangent[2] = f * (delta_uv2[1] * edge1[2] - delta_uv1[1] * edge2[2]);

            bitangent[0] = f * (-delta_uv2[0] * edge1[0] + delta_uv1[0] * edge2[0]);
            bitangent[1] = f * (-delta_uv2[0] * edge1[1] + delta_uv1[0] * edge2[1]);
            bitangent[2] = f * (-delta_uv2[0] * edge1[2] + delta_uv1[0] * edge2[2]);

            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);
            veclistpush(&seg->tangents, tangent);

            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);
            veclistpush(&seg->bitangents, bitangent);

            /* Index calculation */
            index[0] = indexcount + 0;
            index[1] = indexcount + 1;
            index[2] = indexcount + 2;
            fastvboaddindex(vbo, index);

            index[0] = indexcount + 2;
            index[1] = indexcount + 3;
            index[2] = indexcount + 0;
            fastvboaddindex(vbo, index);

            indexcount += 4;
            vbo->indxcount += 6;
        }
    }
}

void
subtilemesh(World *world, Segment *seg, uint x, uint y, uint z)
{
    uint i, j, vertidx, uvidx, normidx, indexcount, world_x, world_z;
    Tile *tile = &seg->layers[y].subtiles[z + x * SEGMENT_SIZE];
    FastVBO *vbo = &seg->vbo;
    vec3 vert;
    vec2 uv;
    vec3 norm;
    ivec3 index;
    Tile *tile_adj[4] = {NULL, NULL, NULL, NULL};

    vec3 verts[4];
    vec3 uvs[4];
    vec3 bitangent, tangent;
    vec3 edge1, edge2;
    vec2 delta_uv1, delta_uv2;
    float f;

    
    world_x = seg->x * SEGMENT_SIZE + x;
    world_z = seg->z * SEGMENT_SIZE + z;

    if (world_z < WORLD_SIZE - 1) { 
        tile_adj[0] = worldgettoptileat(world, world_x, y, world_z + 1);
    }
    tile_adj[1] = worldgettoptileat(world, world_x, y, world_z - 1);
    tile_adj[2] = worldgettoptileat(world, world_x - 1, y, world_z);

    if (world_x < WORLD_SIZE - 1) {
        tile_adj[3] = worldgettoptileat(world, world_x + 1, y, world_z);
    }
    indexcount = (vbo->indxcount * 2) / 3;

    for (i = 0; i < 5; i++) {
        normidx = i * 3;

        switch (i) {
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
            vert[0] = subfloorverts[vertidx + 0] + x;
            vert[1] = subfloorverts[vertidx + 1] + y;
            vert[2] = subfloorverts[vertidx + 2] + z;

            if (i < 4) {
                memcpy(uv, tile->data->sidetex + uvidx, 2 * sizeof(float));
            } else {
                memcpy(uv, tile->data->toptex + uvidx, 2 * sizeof(float));
            }
            memcpy(norm, worldnorms + normidx, 3 * sizeof(float));
            memcpy(verts[j], vert, 3 * sizeof(float));
            memcpy(uvs[j], uv, 2 * sizeof(float));
            fastvboaddvertex(vbo, vert, uv, norm);
        }
        /* Tangent and Bi-tangent calculation */
        glm_vec3_sub(verts[1], verts[0], edge1);
        glm_vec3_sub(verts[2], verts[0], edge2);

        glm_vec2_sub(uvs[1], uvs[0], delta_uv1);
        glm_vec2_sub(uvs[2], uvs[0], delta_uv2);

        f = 1.0f / (delta_uv1[0] * delta_uv2[1] - delta_uv2[0] * delta_uv1[1]);

        tangent[0] = f * (delta_uv2[1] * edge1[0] - delta_uv1[1] * edge2[0]);
        tangent[1] = f * (delta_uv2[1] * edge1[1] - delta_uv1[1] * edge2[1]);
        tangent[2] = f * (delta_uv2[1] * edge1[2] - delta_uv1[1] * edge2[2]);

        bitangent[0] = f * (-delta_uv2[0] * edge1[0] + delta_uv1[0] * edge2[0]);
        bitangent[1] = f * (-delta_uv2[0] * edge1[1] + delta_uv1[0] * edge2[1]);
        bitangent[2] = f * (-delta_uv2[0] * edge1[2] + delta_uv1[0] * edge2[2]);

        veclistpush(&seg->tangents, tangent);
        veclistpush(&seg->tangents, tangent);
        veclistpush(&seg->tangents, tangent);
        veclistpush(&seg->tangents, tangent);

        veclistpush(&seg->bitangents, bitangent);
        veclistpush(&seg->bitangents, bitangent);
        veclistpush(&seg->bitangents, bitangent);
        veclistpush(&seg->bitangents, bitangent);

        /* Index calculation */
        index[0] = indexcount + 0;
        index[1] = indexcount + 1;
        index[2] = indexcount + 2;
        fastvboaddindex(vbo, index);

        index[0] = indexcount + 2;
        index[1] = indexcount + 3;
        index[2] = indexcount + 0;
        fastvboaddindex(vbo, index);

        indexcount += 4;
        vbo->indxcount += 6;
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
                tile = layer->toptiles + z + x * SEGMENT_SIZE;

                if (tile->data) {
                    toptilemesh(world, seg, x, y, z);
                } else {
                    tile = layer->subtiles + z + x * SEGMENT_SIZE;
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
segmentlightmapjob(void *arg)
{
    Segment *seg;
    World *world;
    struct SegJobArgs *args = arg;

    seg = args->seg;
    world = args->world;

    segmentcalclights(world, seg);

    free(args);
}

void
segmentmeshjob(void *arg)
{
    struct SegJobArgs *args = arg;

    Segment *seg = args->seg;
    World *world = args->world;

    segmentmesh(world, seg);

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

void segmentqueuelightmap(World *world, Segment *seg)
{
    struct SegJobArgs *args;

    args = malloc(sizeof(struct SegJobArgs));
    args->seg = seg;
    args->world = world;

    tpooladdjob(workerpool, segmentlightmapjob, args);
}

void
worldrender(World *world, Camera *cam)
{
    Agent *player = agentgetplayer();
    Segment *seg;
    List *seglist;
    uint pseg_x, pseg_z, idx;
    int i, j, k, seg_x, seg_z, light_count;
    clock_t time;

    seglist = listcreate();
    pseg_x = player->ent.x / SEGMENT_SIZE;
    pseg_z = player->ent.z / SEGMENT_SIZE;

    spacebackgroundrender(cam);

    texsetactivetexture(0);
    texbind(tilegetalbedotex());
    texsetactivetexture(1);
    texbind(tilegetemissiontex());
    texsetactivetexture(2);
    texbind(tilegetnormaltex());

    shaderbind(&worldshader);
    shaderloadprojviewmat(&worldshader, cam->projviewmat);
    shaderloadviewmat(&worldshader, cam->viewmat);

    shaderaddint(&worldshader, "albedo_map", 0);
    shaderaddint(&worldshader, "emission_map", 1);
    shaderaddint(&worldshader, "normal_map", 2);
    shaderaddint(&worldshader, "light_map", 3);

    vec3 viewpos = {cam->ent.x, cam->ent.y, cam->ent.z};
    shaderaddvec3(&worldshader, "viewpos", viewpos);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    const int MAX_LIGHTS = 1024;
    vec3 lightpos[MAX_LIGHTS];
    vec3 lightcolor[MAX_LIGHTS];
    light_count = 0;

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
            listpushback(seglist, seg);

            for (k = 0; k < (int)seg->lights->size; k++, light_count++) {
                Light *light = listat(seg->lights, k);
                lightpos[light_count][0] = light->pos[0];
                lightpos[light_count][1] = light->pos[1] + 1.1f;
                lightpos[light_count][2] = light->pos[2];
                lightcolor[light_count][0] = light->color[0];
                lightcolor[light_count][1] = light->color[1];
                lightcolor[light_count][2] = light->color[2];
            }
        }
    }
    glUniform3fv(glGetUniformLocation(worldshader.prog, "light_pos"), light_count, (float *)lightpos);
    glUniform3fv(glGetUniformLocation(worldshader.prog, "light_color"), light_count, (float *)lightcolor);
    shaderaddint(&worldshader, "light_count", light_count);

    time = clock();
    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);
        if (seg->dirty) {
            fastvboreset(&seg->vbo);
            veclistclear(&seg->tangents);
            veclistclear(&seg->bitangents);
        }
    }

    /* Build Lightmaps */
    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);
        
        if (seg->relight) {
            segmentqueuelightmap(world, seg);
        }
    }
    tpoolwait(workerpool);

    /*for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);

        if (seg->relight) {
            segmentqueuerelight(world, seg);
        }
    }
    tpoolwait(workerpool);*/

    /* Write lightmaps to segment texture lightmap */
    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);

        if (seg->relight) {
            segmentrelight(world, seg);
        }
    }
    /* Clear all flags, finishing light updates */
    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);
        segmentunflaglights(world, seg);
    }


    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);

        if (seg->dirty) {
            segmentqueueremesh(world, seg);
        }
    }
    tpoolwait(workerpool);
    time = clock() - time;

    int meshed = 0;

    for (idx = 0; idx < seglist->size; idx++) {
        seg = listat(seglist, idx);
        
        if (seg->dirty) {
            fastvbobind(&seg->vbo);
            fastvbobindvbo(&seg->vbo, &seg->tangents);
            fastvbobindvbo(&seg->vbo, &seg->bitangents);
            seg->dirty = 0;
            meshed = 1;
        }
        texsetactivetexture(3);
        glBindTexture(GL_TEXTURE_2D, seg->light_tex.id);

        glBindVertexArray(seg->vbo.vao);
        shaderloadmodelmat(&worldshader, seg->modelmat);
        glDrawElements(GL_TRIANGLES, seg->vbo.indxcount, GL_UNSIGNED_INT, NULL);
    }
    if (meshed) {
        logprint(LOG_INFO, "Rebuilt meshes in $fs", time / (float)CLOCKS_PER_SEC);
    }
    shaderunbind();
    selectionrender(cam);
    listdelete(seglist);
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
    glDrawElements(GL_TRIANGLES, space_bg.vbo.indxcount, GL_UNSIGNED_INT, NULL);
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
    glDrawElements(GL_LINES, selection.vbo.indxcount, GL_UNSIGNED_INT, NULL);
    shaderunbind();
}

void
worldaddlight(World *world, float x, float y, float z,
        float r, float g, float b, int radius)
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
            seg = worldgetsegmentat(world, x + i * SEGMENT_SIZE, z + j * SEGMENT_SIZE);
            seg->relight = 1;
        }
    }
}

void
worldremovelight(World *world, uint x, uint y, uint z)
{
    uint i; 
    int j, k;
    Segment *seg;
    Light *light;

    seg = worldgetsegmentat(world, x, z);

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);

        if ((uint)light->pos[0] == x &&
                (uint)light->pos[1] == y &&
                (uint)light->pos[2] == z) {
            listremove(seg->lights, i);
            lightdelete(light);
            seg->relight = 1;
            
            for (j = -1; j <= 1; j++) {
                for (k = -1; k <= 1; k++) {
                    seg = worldgetsegmentat(world,
                            x + j * SEGMENT_SIZE,
                            z + k * SEGMENT_SIZE);
                    seg->relight = 1;
                }
            }
        }
    }
}

List *
segmentgetadjlights(World *world, Segment *seg)
{
    uint l, seg_x, seg_z;
    int x, z;
    Segment *seg_adj;
    Light *light;
    List *result;

    result = listcreate();

    for (x = -1; x <= 1; x++) {
        for (z = -1; z <= 1; z++) {
            seg_x = seg->x + x;
            seg_z = seg->z + z;

            if (seg_x > SEGMENT_COUNT || seg_z > SEGMENT_COUNT) {
                continue;
            }
            seg_adj = &world->segs[seg_z + seg_x * SEGMENT_COUNT];

            for (l = 0; l < seg_adj->lights->size; l++) {
                light = listat(seg_adj->lights, l);

                if (light->pos[0] <= seg->x * SEGMENT_SIZE - LIGHT_MAX_RADIUS ||
                        light->pos[0] >= (seg->x + 1) * SEGMENT_SIZE + LIGHT_MAX_RADIUS - 1 ||
                        light->pos[2] <= seg->z * SEGMENT_SIZE - LIGHT_MAX_RADIUS ||
                        light->pos[2] >= (seg->z + 1) * SEGMENT_SIZE + LIGHT_MAX_RADIUS - 1) {
                    continue;
                }
                listpushback(result, light);
            }
        }
    }

    return result;
}

/* Calculates the lightmaps of every 'dirty' light in the segment */
void
segmentcalclights(World *world, Segment *seg)
{
    uint i;
    Light *light;

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);
        
        if (light->dirty) {
            lightcalclightmap(light, world);
        }
    }
}

/* Applies all modified lights to the segment's lightmap texture */
void
segmentrelight(World *world, Segment *seg)
{
    uint i;
    List *lightlist;
    Light *light;

    lightlist = segmentgetadjlights(world, seg);

    for (i = 0; i < lightlist->size; i++) {
        light = listat(lightlist, i);
        
        if (light->dirty) {
            lightapplytosegment(listat(lightlist, i), seg);
        }
    }

    listdelete(lightlist);
}

/* Sets flags on segment and segment lights to recalculate all lightmaps */
void
segmentflagfullrelight(World *world, Segment *seg)
{
    uint i;
    Light *light;

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);
        light->dirty = 1;
    }
    seg->relight = 1;
}

/* Unflags all segment lights for relighting */
void
segmentunflaglights(World *world, Segment *seg)
{
    uint i;
    Light *light;

    for (i = 0; i < seg->lights->size; i++) {
        light = listat(seg->lights, i);
        light->dirty = 0;
    }
    seg->relight = 0;
}

/* Relights*/

/*
void
lightinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner)
{
    uint seg_x, seg_z;
    Segment *seg = worldgetsegmentat(world, x, z);
    Layer *layer = seg->layers + y;
    LightColor *vertlight;
    
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
    seg_x = x % SEGMENT_SIZE;
    seg_z = z % SEGMENT_SIZE;

    switch (corner) {
    case TILE_BR:
        vertlight = layer->lightmap +
            (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        break;
    case TILE_BL:
        vertlight = layer->lightmap +
            (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        break;
    case TILE_TL:
        vertlight = layer->lightmap +
            (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        break;
    case TILE_TR:
        vertlight = layer->lightmap +
            (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        break;
    }
    memcpy(color, vertlight, sizeof(LightColor));
}

void
lightwallsideinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner)
{
    uint isleft, seg_x, seg_z;
    Segment *seg = worldgetsegmentat(world, x, z);
    Layer *layer = seg->layers + y;
    LightColor *vertlight;
    
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
    isleft = corner == TILE_BL || corner == TILE_TL;
    seg_x = x % SEGMENT_SIZE;
    seg_z = z % SEGMENT_SIZE;

    switch (side) {
    case WALL_FRONT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_BACK:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_LEFT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_RIGHT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        }
        break;
    }
    memcpy(color, vertlight, sizeof(LightColor));
}

void 
lightwalltopinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner)
{
    uint seg_x, seg_z;
    Segment *seg = worldgetsegmentat(world, x, z);
    Layer *layer = seg->layers + y;
    LightColor *vertlight;
    
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
    seg_x = x % SEGMENT_SIZE;
    seg_z = z % SEGMENT_SIZE;

    switch (corner) {
    case TILE_BR:
        vertlight = layer->lightmap +
            (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        break;
    case TILE_BL:
        vertlight = layer->lightmap +
            (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        break;
    case TILE_TL:
        vertlight = layer->lightmap +
            (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        break;
    case TILE_TR:
        vertlight = layer->lightmap +
            (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        break;
    }
    memcpy(color, vertlight, sizeof(LightColor));
    color[0] *= 0.8f;
    color[1] *= 0.8f;
    color[2] *= 0.8f;
}

void
lightwallsubinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner)
{
    uint isleft, seg_x, seg_z;
    Segment *seg = worldgetsegmentat(world, x, z);
    Layer *layer = seg->layers + y;
    LightColor *vertlight;

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
    isleft = corner == TILE_BL || corner == TILE_TL;
    seg_x = x % SEGMENT_SIZE;
    seg_z = z % SEGMENT_SIZE;
    side -= 5;

    switch (side) {
    case WALL_FRONT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_BACK:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_LEFT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 0) * (SEGMENT_SIZE + 1);
        }
        break;
    case WALL_RIGHT:
        if (isleft) {
            vertlight = layer->lightmap +
                (seg_z + 1) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        } else {
            vertlight = layer->lightmap +
                (seg_z + 0) + (seg_x + 1) * (SEGMENT_SIZE + 1);
        }
        break;
    }
    memcpy(color, vertlight, sizeof(LightColor));
    color[0] *= 0.8f;
    color[1] *= 0.8f;
    color[2] *= 0.8f;
}
*/
Segment *
worldgetsegmentat(World *world, uint x, uint z)
{
    x /= SEGMENT_SIZE;
    z /= SEGMENT_SIZE;

    return world->segs + z + x * SEGMENT_COUNT;
}

Layer *
worldgetlayerat(World *world, uint x, uint y, uint z)
{
    x /= SEGMENT_SIZE;
    z /= SEGMENT_SIZE;

    return &world->segs[z + x * SEGMENT_COUNT].layers[y];
}
/*
LightColor *
worldgetlightcellat(World *world, uint x, uint y, uint z)
{
    Layer *layer = worldgetlayerat(world, x, y, z);

    x %= SEGMENT_SIZE + 1;
    z %= SEGMENT_SIZE + 1;

    return layer->lightmap + z + x * (SEGMENT_SIZE + 1);
}
*/
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

    return layer->subtiles + z + x * SEGMENT_SIZE;
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
