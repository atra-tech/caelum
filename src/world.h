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

#include "util/util.h"
#include "render.h"
#include "tile.h"

#define WORLD_SIZE      1024
#define WORLD_HEIGHT    1
#define SEGMENT_SIZE    16
#define LAYER_TILECOUNT SEGMENT_SIZE * SEGMENT_SIZE
#define SEGMENT_COUNT   WORLD_SIZE / SEGMENT_SIZE

struct LightColor;

typedef struct Layer {
    uint pos;
    Tile *toptiles;
    Tile *subtiles;
    struct LightColor *light;
    //double *pressures;
    //double *temps;
    //List *gases; List of gases {type: uint, amount: double}
    //List *links; Transportation elements between top and sub tiles
    //List *ents; Tile grid bound entities
} Layer;

typedef struct Segment {
    uint x, z, dirty, relight;
    Layer *layers;
    VBuffer vbo;
    List *vertlights; /* VBO light data */
    mat4 modelmat;
    List *lights; /* Worldspace light entities */
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
struct LightColor *worldgetlightcellat(World *world, uint x, uint y, uint z);
Tile *worldgettoptileat(World *world, uint x, uint y, uint z);
Tile *worldgetsubtileat(World *world, uint x, uint y, uint z);
Tile *segmentgettoptile(Segment *seg, uint x, uint y, uint z);
Tile *segmentgetsubtile(Segment *seg, uint x, uint y, uint z);

void worldaddlight(World *world, uint x, uint y, uint z,
        float r, float g, float b, uint radius);
void worldremovelight(World *world, uint x, uint y, uint z);
void segmentrelight(World *world, Segment *seg);

#endif /* CAELUM_WORLD_H */
