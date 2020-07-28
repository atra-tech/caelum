/*
*  CAELUM (2020)
*
*  Copyright Robert Saarloos, All rights reserved.
*
*  Describes light and light related operations for roguelike
*  voxel-based lighting.
*
* */

#include <xmmintrin.h>

#include "light.h"
#include "world.h"
#include "util/log.h"
//#include "tile.h"

static const int mults[4][8] = {
    {1, 0, 0, -1, -1, 0, 0, 1},
    {0, 1, -1, 0, 0, -1, 1, 0},
    {0, 1, 1, 0, 0, -1, -1, 0},
    {1, 0, 0, 1, -1, 0, 0, -1}
};

static void illuminate(Light *light, uint x, uint y, uint r2, uint dist2,
        float mult);
static void castlight(Light *light, World *world, uint row, float startslope,
        float endslope, uint xx, uint xy, uint yx, uint yy);
static void lightapplytilelight(Light *light, Layer *layer,
        uint x, uint z, uint wx, uint wz);

Light *
lightcreate(float x, float y, float z, uint radius)
{
    Light *light = malloc(sizeof(Light));
    uint mapsize;
    
    mapsize = radius * 2 + 1;
    mapsize *= mapsize;

    light->x = x;
    light->y = y;
    light->z = z;
    light->color.r = 0.0f;
    light->color.g = 0.0f;
    light->color.b = 0.0f;
    light->radius = radius;
    light->lightmap = calloc(mapsize, sizeof(LightColor));

    return light;
}

void
lightsetcolor(Light *light, float r, float g, float b)
{
    light->color.r = r;
    light->color.g = g;
    light->color.b = b;
}

void
lightcalclightmap(Light *light, World *world)
{
    uint i, x, y, mapsize;
    LightColor *cell;

    mapsize = light->radius * 2 + 1;
    mapsize *= mapsize;

    memset(light->lightmap, 0, mapsize * sizeof(LightColor));

    for (i = 0; i < 8; i++) {
        castlight(light, world, 1, 1.0, 0.0, mults[0][i], mults[1][i],
                mults[2][i], mults[3][i]);
    }
    x = light->radius;
    y = light->radius;
    cell = &light->lightmap[y + x * (light->radius * 2 + 1)];
    memcpy(cell, &light->color, sizeof(LightColor));
}

int
lightinradius(int lx, int lz, int x, int z, int radius)
{
    int dx, dz;

    dx = lx - x;
    dz = lz - z;

    return dx * dx + dz * dz < radius * radius;
}

void
lightclearsegment(Segment *seg)
{
    uint y;
    Layer *layer;

    for (y = 0; y < WORLD_HEIGHT; y++) {
        layer = &seg->layers[y];
        memset(layer->light, 0, LAYER_TILECOUNT * sizeof(LightColor));
    }
}

void
lightapplytilelight(Light *light, Layer *layer,
        uint x, uint z, uint wx, uint wz)
{
    uint map_x, map_z, mapsize;
    float *dest, *src;

    if (!lightinradius(light->x, light->z, wx, wz, light->radius)) {
        return;
    }
    mapsize = light->radius * 2 + 1;
    map_x = wx - (light->x - light->radius);
    map_z = wz - (light->z - light->radius);

    src = (float *)&light->lightmap[map_z + map_x * mapsize];
    dest = (float *)&layer->light[z + x * SEGMENT_SIZE];

    glm_vec3_add(dest, src, dest);
    /*dest->r += src->r;
    dest->g += src->g;
    dest->b += src->b;*/
}

void
lightapplytosegment(List *lights, Segment *seg)
{
    const uint x_offset = seg->x * SEGMENT_SIZE;
    const uint z_offset = seg->z * SEGMENT_SIZE;
    uint seg_x, seg_z, tile_x, tile_z, l_idx, y;
    Layer *layer;
    Light *light;

    for (y = 0; y < WORLD_HEIGHT; y++) {
        layer = &seg->layers[y];

        for (seg_x = 0; seg_x < SEGMENT_SIZE; seg_x++) {
            for (seg_z = 0; seg_z < SEGMENT_SIZE; seg_z++) {
                tile_x = seg_x + x_offset;
                tile_z = seg_z + z_offset;
                
                for (l_idx = 0; l_idx < lights->size; l_idx++) {
                    light = listat(lights, l_idx);
                    lightapplytilelight(light, layer,
                            seg_x, seg_z, tile_x, tile_z);
                }
            }
        }
    }
}

void
lightlayercalcmap(struct World *world, struct Layer *layer)
{
    uint x, z;
}

void
castlight(Light *light, World *world, uint row, float startslope,
        float endslope, uint xx, uint xy, uint yx, uint yy)
{
    float startslopenext, lslope, rslope;
    int dx, dy, sax, say, blocked;
    uint i, ax, ay, radius2, dist2;
    Tile *tile;

    if (startslope <= endslope) {
        return;
    }
    startslopenext = startslope;

    for (i = row; i <= light->radius; i++) {
        blocked = 0;

        for (dx = -i, dy = -i; dx <= 0; dx++) {
            lslope = (dx - 0.5) / (dy + 0.5);
            rslope = (dx + 0.5) / (dy - 0.5);

            if (startslope <= rslope) {
                continue;
            } else if (endslope > lslope) {
                break;
            }
            sax = dx * xx + dy * xy;
            say = dx * yx + dy * yy;

            if ((sax < 0 && abs(sax) > light->x) ||
                    (say < 0 && abs(say) > light->z)) {
                continue;
            }
            ax = light->x + sax;
            ay = light->z + say;

            if (ax >= WORLD_SIZE || ay >= WORLD_SIZE) {
                continue;
            }
            radius2 = light->radius * light->radius;
            dist2 = dx * dx + dy * dy;
            tile = worldgettoptileat(world, ax, light->y, ay);
            
            if (dist2 < radius2) {
                if (tile->data && tile->data->attr & TILE_WALL) {
                    //illuminate(light, ax, ay, radius2, dist2, 0.4);
                } else {
                    illuminate(light, ax, ay, radius2, dist2, 1.0);
                }
            }

            if (blocked) {
                if (tile->data) {
                    if (tile->data->attr & TILE_WALL) {
                        startslopenext = rslope;
                        continue;
                    } else {
                        blocked = 0;
                        startslope = startslopenext;
                    }
                } else {
                    blocked = 0;
                    startslope = startslopenext;
                }
            } else if (tile->data) {
                if (tile->data->attr & TILE_WALL) {
                    blocked = 1;
                    startslopenext = rslope;
                    castlight(light, world, i + 1, startslope, lslope,
                            xx, xy, yx, yy);
                }
            }
        }
        if (blocked) {
            break;
        }
    }
}

void
illuminate(Light *light, uint x, uint y, uint r2, uint dist2, float mult)
{
    float intensity = 1.0f - ((float)dist2 / (float)r2);
    uint lx = x - (light->x - light->radius);
    uint ly = y - (light->z - light->radius);
    LightColor *cell = &light->lightmap[ly + lx * (light->radius * 2 + 1)];
    
    intensity *= mult;

    cell->r = light->color.r * intensity;
    cell->g = light->color.g * intensity;
    cell->b = light->color.b * intensity;
}

void
lightdelete(Light *light)
{
    free(light->lightmap);
    free(light);
}
