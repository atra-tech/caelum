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

typedef struct LightColor LightColor;

typedef struct Layer {
    uint pos;
    Tile *toptiles;
    Tile *subtiles;
    LightColor *light;
    //double *pressures;
    //double *temps;
    //List *gases; List of gases {type: uint, amount: double}
    //List *links; Transportation elements between top and sub tiles
    //List *ents; Tile grid bound entities
} Layer;

typedef struct Segment {
    uint x, z;
    Layer *layers;
    VBuffer vbo;
    List *vertlights;
    mat4 modelmat;
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
LightColor *worldgetlightcellat(World *world, uint x, uint y, uint z);
Tile *worldgettoptileat(World *world, uint x, uint y, uint z);
Tile *worldgetsubtileat(World *world, uint x, uint y, uint z);
Tile *segmentgettoptile(Segment *seg, uint x, uint y, uint z);
Tile *segmentgetsubtile(Segment *seg, uint x, uint y, uint z);

#endif /* CAELUM_WORLD_H */
