/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Describes the game world, tiles, tile-bound entities and 
 *  related functions.
 *
 * */

#ifndef CAELUM_WORLD_H
#define CAELUM_WORLD_H

#include <time.h>

#include "util/util.h"
#include "render.h"
#include "tile.h"
#include "light.h"

#define WORLD_SIZE      1024
#define WORLD_HEIGHT    1
#define SEGMENT_SIZE    16
#define LAYER_TILECOUNT SEGMENT_SIZE * SEGMENT_SIZE
#define SEGMENT_COUNT   WORLD_SIZE / SEGMENT_SIZE
#define VIEW_DIST       5
#define VIEW_DIST_AREA  25

typedef struct Layer {
    uint pos;
    Tile *toptiles;
    Tile *subtiles;
} Layer;

typedef struct Segment {
    uint x, z, dirty, relight;
    Layer *layers;
    FastVBO vbo;
    VecList tangents;
    VecList bitangents;
    mat4 modelmat;
    List *lights;           /* Worldspace light entities */
    LightTex light_tex;     /* OpenGL texture of light_map */
} Segment;

typedef struct World {
    Segment *segs;
} World;

void worldinit(void);
void worldshutdown(void);

World *worldcreate(void);
void worldhandle(World *world, Event *e);
void worldupdate(World *world, float dt);
void worldrender(World *world, Camera *cam);
void worlddelete(World *world);

Segment *worldgetsegmentat(World *world, uint x, uint z);
Layer *worldgetlayerat(World *world, uint x, uint y, uint z);
Tile *worldgettoptileat(World *world, uint x, uint y, uint z);
Tile *worldgetsubtileat(World *world, uint x, uint y, uint z);
Tile *segmentgettoptile(Segment *seg, uint x, uint y, uint z);
Tile *segmentgetsubtile(Segment *seg, uint x, uint y, uint z);
List *segmentgetadjlights(World *world, Segment *seg);

void worldaddlight(World *world, float x, float y, float z,
        float r, float g, float b, int radius);
void worldremovelight(World *world, uint x, uint y, uint z);
void segmentrelight(World *world, Segment *seg);
void segmentcalclights(World *world, Segment *seg);

void segmentflagfullrelight(World *world, Segment *seg);
void segmentunflaglights(World *world, Segment *seg);
void segmentflagrelightat(World *world, Segment *seg, int x, int z);

#endif /* CAELUM_WORLD_H */
