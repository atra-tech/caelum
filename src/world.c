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

static Segment *segmentcreatematrix(uint rows, uint cols);
static Layer *layercreatearray(uint count);
static void segmentdeletematrix(Segment *segs, uint rows, uint cols);
static void layerdeletearray(Layer *layers, uint count);
static void lightinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsideinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);
static void lightwalltopinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint corner);
static void lightwallsubinterpolate(World *world, uint x, uint y, uint z,
        float *color, uint side, uint corner);

static List *tiledata;
static Shader worldshader;

void
worldinit()
{
    tiledatabaseinit("res/tile/manifest");
    tiledata = listcreate();
    shaderinit(&worldshader,
            "res/shader/worldvert.glsl",
            "res/shader/worldfrag.glsl");
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
            data = tilegetdata("res/tile/maint_floor.abf");

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
            segs[idx].layers = layercreatearray(WORLD_HEIGHT);
            vbufinit(&segs[idx].vbo);
            segs[idx].vertlights = listcreate();
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
    free(world);
}

void
segmentdeletematrix(Segment *segs, uint rows, uint cols)
{
    uint i, segcount = rows * cols;

    for (i = 0; i < segcount; i++) {
        layerdeletearray(segs[i].layers, WORLD_HEIGHT);
        vbufdelete(&segs[i].vbo);
        listdelete(segs[i].vertlights);
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
worldhandle(World *world, Event *e)
{
    //  Handle click events or key presses on world objects.
    //  TODO: tile picking algorithm
}

void
worldupdate(World *world, float dt)
{
    //  Update pressure and temperature values and air concentration
    //  Step pipe, wire values
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
    world_z = seg->x * SEGMENT_SIZE + z;

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
                /*colorvec[0] = color->r;
                colorvec[1] = color->g;
                colorvec[2] = color->b;*/
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
    y -= 0.1f;
    color = &seg->layers[y].light[z + x * SEGMENT_SIZE];

/*    for (k = 0; k < 20; k++) {
        float *colorvec = malloc(3 * sizeof(float));
        uint corner = k % 4;
        colorvec[0] = color->r;
        colorvec[1] = color->g;
        colorvec[2] = color->b;
        lightinterpolate(world, seg->x * SEGMENT_SIZE + x, y,
                seg->z * SEGMENT_SIZE + z, colorvec, corner);
        listpushback(seg->vertlights, colorvec);
    }*/
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

    vbufreset(&seg->vbo);
    listdealloc(seg->vertlights);
    listclear(seg->vertlights);

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
    logprint(LOG_INFO, "Meshed segment");
    vbufbind(&seg->vbo);
    vbufbindvbo(&seg->vbo, 3, seg->vertlights);
}

void
worldrender(World *world, Camera *cam)
{
    static uint playerseg[2] = {128, 128};
    Agent *player = agentgetplayer();
    Segment *seg;
    uint pseg_x, pseg_z, remesh;
    int i, j, seg_x, seg_z;

    pseg_x = player->ent.x / SEGMENT_SIZE;
    pseg_z = player->ent.z / SEGMENT_SIZE;
    remesh = (pseg_x != playerseg[0] || pseg_z != playerseg[1]);
    playerseg[0] = pseg_x;
    playerseg[1] = pseg_z;

    texsetactivetexture(0);
    texbind(tilegetalbedotex());
    texsetactivetexture(1);
    texbind(tilegetemissiontex());
    //texsetactivetexture(2);
    //texbind(tilegetnormaltex());

    shaderbind(&worldshader);
    shaderloadprojviewmat(&worldshader, cam->projviewmat);
    shaderloadviewmat(&worldshader, cam->viewmat);

    //shaderloadint(glGetUniformLocation(worldshader.prog, "normal_map"), 2);
    
    shaderaddint(&worldshader, "albedo_map", 0);
    shaderaddint(&worldshader, "emission_map", 1);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    for (i = -1; i <= 1; i++) {
        seg_x = pseg_x + i;
        if (seg_x < 0 || seg_x >= SEGMENT_COUNT) {
            continue;
        }

        for (j = -1; j <= 1; j++) {
            seg_z = pseg_z + j;
            seg = &world->segs[seg_z + seg_x * SEGMENT_COUNT];
            
            if (seg_z < 0 || seg_z >= SEGMENT_COUNT) {
                continue;
            }

            if (remesh) {
                segmentmesh(world, seg);
            }
            glBindVertexArray(seg->vbo.vao);
            shaderloadmodelmat(&worldshader, seg->modelmat);
            glDrawElements(GL_TRIANGLES, seg->vbo.indxcount,
                    GL_UNSIGNED_INT, NULL);
        }
    }
    shaderunbind();
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

    color[0] *= 0.25;
    color[1] *= 0.25;
    color[2] *= 0.25;
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
