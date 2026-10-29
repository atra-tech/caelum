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

static inline void illuminate(Light *light, int x, int y);
static void castlight(Light *light, World *world, int row, float startslope,
        float endslope, int xx, int xy, int yx, int yy);

Light *
lightcreate(float x, float y, float z, int radius)
{
    Light *light = malloc(sizeof(Light));
    const int mapsize = radius * 2 * LIGHT_RESOLUTION;
    
    light->pos[0] = x;
    light->pos[1] = y;
    light->pos[2] = z;
    light->active = 1;
    light->color[0] = 0.0f;
    light->color[1] = 0.0f;
    light->color[2] = 0.0f;
    light->radius = radius;
    light->dirty = 1;
    light->map = lightmap_create(mapsize, mapsize);

    return light;
}

void
lightsetcolor(Light *light, float r, float g, float b)
{
    light->color[0] = r;
    light->color[1] = g;
    light->color[2] = b;
}

void
lightcalclightmap(Light *light, World *world)
{
    int i, x, y, mapsize, mapwidth;
    int *cell;

    mapwidth = light->radius * 2 * LIGHT_RESOLUTION;
    mapsize = mapwidth * mapwidth;

    memset(light->map.pixels, 0, mapsize);

    for (i = 0; i < 8; i++) {
        castlight(light, world, 1, 1.0, 0.0,
                mults[0][i], mults[1][i],
                mults[2][i], mults[3][i]);
    }

    /* Set the immediate surrounding cells */
    x = light->radius * 4 - 1;
    y = light->radius * 4 - 1;
    cell = light->map.pixels + x + y * mapwidth;
    *cell = 0xFFFFFFFF; cell += 1;
    *cell = 0xFFFFFFFF; cell += light->radius * 2 * LIGHT_RESOLUTION;
    *cell = 0xFFFFFFFF; cell -= 1;
    *cell = 0xFFFFFFFF;
}

int
lightinradius(int lx, int lz, int x, int z, int radius)
{
    int dx, dz;

    dx = lx - x;
    dz = lz - z;

    return dx * dx + dz * dz < radius * radius;
}
/*
void
lightclearsegment(Segment *seg)
{
    uint y, mapsize;
    Layer *layer;

    mapsize = SEGMENT_SIZE + 1;
    mapsize *= mapsize;

    for (y = 0; y < WORLD_HEIGHT; y++) {
        layer = seg->layers + y;
        memset(layer->lightmap, 0, mapsize * sizeof(LightColor));

    }
}
*/
/*
void
lightapplytilelight(Light *light, Layer *layer,
        uint x, uint z, uint wx, uint wz)
{
    uint map_x, map_z, mapsize;
    LightColor *dest, *src;

    if (!lightinradius(light->x, light->z, wx, wz, light->radius)) {
        return;
    }
    mapsize = light->radius * 2 + 2;
    map_x = wx - (light->x - light->radius);
    map_z = wz - (light->z - light->radius);

    src = light->lightmap + map_z + map_x * mapsize;
    dest = layer->lightmap + z + x * (SEGMENT_SIZE + 1);

    dest->r += src->r;
    dest->g += src->g;
    dest->b += src->b;
}
*/

static inline Tile *
lightgetworldspacetile(World *restrict world, uint x, uint z)
{
    //return worldgettoptileat(world, x >> 2, 0, z >> 2);
    return worldgettoptileat(world, x, 0, z);
}

/* Writes a light's lightmap image to a segment's lightmap texture */
void
lightapplytosegment(Light *light, Segment *seg)
{
    const int half_seg = SEGMENT_SIZE / 2;
    const int map_size = LIGHT_MAX_RADIUS * 2 * LIGHT_RESOLUTION;

    const int light_x = light->pos[0] - (seg->x * SEGMENT_SIZE) + half_seg;
    const int light_z = light->pos[2] - (seg->z * SEGMENT_SIZE) + half_seg;
    const int map_x = light_x * map_size;
    const int map_y = light_z * map_size;

    glBindTexture(GL_TEXTURE_2D, seg->light_tex.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, map_x, map_y, map_size, map_size,
            GL_BGRA, GL_UNSIGNED_BYTE, light->map.pixels);
}

void
castlight(Light *light, World *world, int row, float startslope,
        float endslope, int xx, int xy, int yx, int yy)
{
    float startslopenext, lslope, rslope;
    int dx, dy, sax, say, ox, oy, blocked, run2;
    int i, ax, ay, radius, radius2, dist2;
    Tile *tile;

    if (startslope <= endslope) {
        return;
    }
    startslopenext = startslope;
    radius = (light->radius) * LIGHT_RESOLUTION - 1;

    for (i = row; i <= radius; i++) {
        blocked = 0;

        for (dx = -i, dy = -i; dx <= 0; dx++) {
            lslope = (dx - 0.5) / (dy + 0.5);
            rslope = (dx + 0.5) / (dy - 0.5);

            if (startslope < rslope) {
                continue;
            } else if (endslope > lslope) {
                break;
            }
            sax = dx * xx + dy * xy;
            say = dx * yx + dy * yy;

            if ((sax < 0 && abs(sax) > light->pos[0]) ||
                    (say < 0 && abs(say) > light->pos[2])) {
                continue;
            }
            ax = light->pos[0] + sax;
            ay = light->pos[2] + say;

            if (ax >= WORLD_SIZE || ay >= WORLD_SIZE) {
                continue;
            }
            radius2 = radius * radius;
            dist2 = dx * dx + dy * dy;
            tile = lightgetworldspacetile(world, light->pos[0] + (sax * 0.25),
                    light->pos[2] + (say * 0.25));
            
            if (dist2 < radius2) {
                if (!(tile->data && tile->data->attr & TILE_WALL)) {
                    ox = sax >= 0;
                    oy = say >= 0;
                    run2 = sax == 0 || say == 0;
                    
                    illuminate(light, ax + ox, ay + oy);
                    ox = sax > 0;
                    oy = say > 0;

                    if (run2) {
                        illuminate(light, ax + ox, ay + oy);
                    }
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
                    castlight(light, world, i + 1, startslope, lslope, xx, xy, yx, yy);
                }
            }
        }
        if (blocked) {
            break;
        }
    }
}

/* Sets the light level of a cell */
static inline void
illuminate(Light *light, int x, int y)
{
    //const int lx = x - (light->pos[0] - light->radius) * LIGHT_RESOLUTION;
    //const int ly = y - (light->pos[2] - light->radius) * LIGHT_RESOLUTION;
    int lx = x - light->pos[0] + light->radius * LIGHT_RESOLUTION;
    int ly = y - light->pos[2] + light->radius * LIGHT_RESOLUTION;

    const int mapsize = light->radius * 2 * LIGHT_RESOLUTION;
    int *cell = light->map.pixels + lx + ly * mapsize;

    if (lx > mapsize || lx < 0 ||
            ly > mapsize || ly < 0) {
        printf("ERROR: Illuminated x=%d, z=%d\n", lx, ly);
    }
    *cell = 0xFFFFFFFF;
}
/*
void
lightsmoothlightmaps(World *world, Segment *seg)
{
    uint idx, l, x, z, mapsize, smooth;
    Light *light;
    LightColor *color;
    LightColor *lightmap;

    for (l = 0; l < seg->lights->size; l++) {
        light = listat(seg->lights, l);
        mapsize = light->radius * 2 + 2;
        lightmap = light->lightmap;

        for (x = 1; x < mapsize - 1; x++) {
            for (z = 1; z < mapsize - 1; z++) {
                idx = z + x * mapsize;
                smooth = 0;
                color = lightmap + idx;

                if (!color->r && !color->g && !color->b) {
                    continue;
                }
                color = lightmap + idx - 1;
                smooth += !color->r && !color->g && !color->b;
                color = lightmap + idx + 1;
                smooth += !color->r && !color->g && !color->b;
                color = lightmap + idx - mapsize;
                smooth += !color->r && !color->g && !color->b;
                color = lightmap + idx + mapsize;
                smooth += !color->r && !color->g && !color->b;
                
                if (smooth) {
                    color = lightmap + idx;
                    color->r *= 0.7f;
                    color->g *= 0.7f;
                    color->b *= 0.7f;
                }
            }
        }
    }
}
*/
void
lightdelete(Light *light)
{
    lightmap_destroy(&light->map);
}

LightMap
lightmap_create(int width, int height)
{
    LightMap map;

    map.width = width;
    map.height = height;
    map.pixels = calloc(width * height, sizeof(int));

    return map;
}

void
lightmap_destroy(LightMap *map)
{
    free(map->pixels);
}

/* Creates a new lightmap texture (VRAM) from an existing lightmap image */
LightTex
lighttex_create(int width, int height)
{
    LightTex tex;

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    
    return tex;
}

/* Unloads the texture from video memory */
void
lighttex_destroy(LightTex *tex)
{
    glDeleteTextures(1, &tex->id);
}

