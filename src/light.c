/*
*  CAELUM (2020)
*
*  Copyright Robert Saarloos, All rights reserved.
*
*  Describes light and light related operations for roguelike
*  voxel-based lighting.
*
* */

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

Light *
lightcreate(float x, float y, float z, uint radius)
{
    Light *light = malloc(sizeof(Light));
    uint mapsize = (radius * 2 + 1) * (radius * 2 + 1);

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
    uint i, x, y;
    LightColor *cell;

    for (i = 0; i < 8; i++) {
        castlight(light, world, 1, 1.0, 0.0, mults[0][i], mults[1][i],
                mults[2][i], mults[3][i]);
    }
    x = light->radius;
    y = light->radius;
    cell = &light->lightmap[x + y * (light->radius * 2 + 1)];
    cell->r = light->color.r;
    cell->g = light->color.g;
    cell->b = light->color.b;

}

void
lightapplylight(Light *light, World *world)
{
    int i, j, x, y, z; 
    int imin, jmin, xmin, zmin, xmax, zmax, imax, jmax;
    int lightmap_size = light->radius * 2 + 1;
    uint zidx, iidx;
    LightColor *lightcell;
    LightColor *cell;
    
    y = light->y;
    imin = 0;
    jmin = 0;
    xmin = light->x - light->radius;
    zmin = light->z - light->radius;
    xmax = light->x + light->radius;
    zmax = light->z + light->radius;
    imax = lightmap_size;
    jmax = lightmap_size;

    if (xmin < 0) {
        jmin = 0 - xmin;
        xmin = 0;
    }
    if (zmin < 0) {
        imin = 0 - zmin;
        zmin = 0;
    }
    if (xmax >= WORLD_SIZE) {
        jmax = lightmap_size - (xmax - (WORLD_SIZE - 1)); 
        xmax = WORLD_SIZE - 1;
    }
    if (zmax >= WORLD_SIZE) {
        imax = lightmap_size - (zmax - (WORLD_SIZE - 1)); 
        zmax = WORLD_SIZE - 1;
    }

    for (i = imin, z = zmin; i < imax; i++, z++) {
        zidx = z * lightmap_size;
        iidx = i * lightmap_size;
        for (j = jmin, x = xmin; j < jmax; j++, x++) {
            lightcell = worldgetlightcellat(world, x, y, z);
            cell = &light->lightmap[j + iidx];

            lightcell->r += cell->r;
            lightcell->g += cell->g;
            lightcell->b += cell->b;
        }
    }
}

void
lightremovelight(Light *light, World *world)
{
    int i, j, x, y, z; 
    int imin, jmin, xmin, zmin, xmax, zmax, imax, jmax;
    int lightmap_size = light->radius * 2 + 1;
    uint zidx, iidx;
    LightColor *lightcell;
    LightColor *cell;
    
    y = light->y;
    imin = 0;
    jmin = 0;
    xmin = light->x - light->radius;
    zmin = light->z - light->radius;
    xmax = light->x + light->radius;
    zmax = light->z + light->radius;
    imax = lightmap_size;
    jmax = lightmap_size;

    if (xmin < 0) {
        jmin = 0 - xmin;
        xmin = 0;
    }
    if (zmin < 0) {
        imin = 0 - zmin;
        zmin = 0;
    }
    if (xmax >= WORLD_SIZE) {
        jmax = lightmap_size - (xmax - (WORLD_SIZE - 1)); 
        xmax = WORLD_SIZE - 1;
    }
    if (zmax >= WORLD_SIZE) {
        imax = lightmap_size - (zmax - (WORLD_SIZE - 1)); 
        zmax = WORLD_SIZE - 1;
    }

    for (i = imin, z = zmin; i < imax; i++, z++) {
        zidx = z * lightmap_size;
        iidx = i * lightmap_size;
        for (j = jmin, x = xmin; j < jmax; j++, x++) {
            lightcell = worldgetlightcellat(world, x, y, z);
            cell = &light->lightmap[j + iidx];

            lightcell->r -= cell->r;
            lightcell->g -= cell->g;
            lightcell->b -= cell->b;
        }
    }
}

void
castlight(Light *light, World *world, uint row, float startslope,
        float endslope, uint xx, uint xy, uint yx, uint yy)
{
    float startslopenext, lslope, rslope;
    int dx, dy, sax, say, blocked;
    uint i, ax, ay, radius2, dist2;
    Tile *tile;

    if (startslope < endslope) {
        return;
    }
    startslopenext = startslope;

    for (i = row; i <= light->radius; i++) {
        blocked = 0;

        for (dx = -i, dy = -i; dx <= 0; dx++) {
            lslope = (dx - 0.5) / (dy + 0.5);
            rslope = (dx + 0.5) / (dy - 0.5);

            if (startslope == rslope) {
                //continue;
            }

            if (startslope < rslope) {
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
    LightColor *cell = &light->lightmap[lx + ly * (light->radius * 2 + 1)];

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
