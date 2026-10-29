/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Describes light and light related operations for roguelike
 *  voxel-based lighting.
 *
 * */

#ifndef CAELUM_LIGHT_H
#define CAELUM_LIGHT_H

#include <cglm/cglm.h>
#include <time.h>

#include "util/util.h"
#include "util/list.h"

#define LIGHT_MAX_RADIUS 8
#define LIGHT_RESOLUTION 4

struct World;
struct Segment;
struct Layer;

typedef struct LightMap {
    int *pixels;       /* Pixel data */
    int width, height;  /* Map dimensions */
} LightMap;

typedef struct LightTex {
    uint id;            /* OpenGL texture ID */
    int width, height;  /* Texture dimensions */
} LightTex;

typedef struct Light {
    vec3 pos;           /* Light position in world space */
    int active;         /* Should the light render? */
    int radius;         /* Maximum light distance */
    int dirty;          /* Lightmap update flag */
    vec3 color;         /* Light color (RGB8) */
    LightMap map;       /* Image containing light data */
} Light;

/* Creates a new light with the given position and radius */
Light * lightcreate(float x, float y, float z, int radius);

/* Sets the RGB color of a light */
void lightsetcolor(Light *light, float r, float g, float b);

/* Calculates the lightmap for a light */
void lightcalclightmap(Light *light, struct World *world);

/* Calculates the lightmap for a light using raycasting */
void lightcalclightmapraycast(Light *light, struct World *world);

/* Sets all pixels in the light's lightmap to 0 */
void lightclearlightmap(Light *light);

/* Writes the light's lightmap to the segment's lightmap texture */
void lightapplytosegment(Light *light, struct Segment *seg);

//void lightclearsegment(struct Segment *seg);

void lightsegmentcalcmap(struct World *, struct Segment *);

//void lightsmoothlightmaps(struct World *world, struct Segment *seg);

void lightdelete(Light *light);


/*
 *  Lightmap image functions
 */

/* Creates a new lightmap image with specified width and height */
LightMap lightmap_create(int width, int height);

/* Frees memory occupied by lightmap */
void lightmap_destroy(LightMap *map);


/*
 *  Lightmap texture functions
 */

/* Reserves space for a new lightmap texture (VRAM) */
LightTex lighttex_create(int width, int height);

/* Unloads the texture from video memory */
void lighttex_destroy(LightTex *tex);

#endif /* CAELUM_LIGHT_H */
