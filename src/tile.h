/*  
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved
 *
 *  Describes tiles and their properties, keeping a central record
 *  of common data of tile types in a hash table.
 *
 * */

#ifndef CAELUM_TILE_H
#define CAELUM_TILE_H

#include "util/util.h"
#include "render.h"

enum {
    TILE_BREAKABLE   = BIT(0),
    TILE_TRANSPARENT = BIT(1),
    TILE_WALL        = BIT(2),
    TILE_FLOOR       = BIT(3),
    TILE_NATURAL     = BIT(4)
};

enum {
    TILE_MAT_GRASS,
    TILE_MAT_STONE,
    TILE_MAT_SAND,
    TILE_MAT_METAL,
    TILE_MAT_GLASS,
    TILE_MAT_FABRIC,
    TILE_MAT_WOOD,
    TILE_MAT_POLISHED
};

typedef struct TileData {
    uint attr;
    uint material;
    uint maxhealth;
    uint toptex_pos[2];
    uint sidetex_pos[2];
    uint subsidetex_pos[2];
    float toptex[8];
    float sidetex[8];
    float subsidetex[8];
} TileData;

typedef struct Tile {
    TileData *data;
    uint health;
} Tile;

void tiledatabaseinit(const char *manifest);
void tiledatabasestop(void);
Texture * tilegetalbedotex(void);
Texture * tilegetnormaltex(void);
Texture * tilegetemissiontex(void);
TileData * tilegetdata(char *tilename);

#endif /* CAELUM_TILE_H */
