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
#include "util/util.h"
#include "util/list.h"
#include <time.h>

struct World;
struct Segment;
struct Layer;

typedef struct LightColor {
    float r, g, b;
} LightColor;

typedef struct Light {
    float x, y, z;
    uint radius;
    LightColor color;
    LightColor *lightmap;
} Light;

Light * lightcreate(float x, float y, float z, uint radius);
void lightsetcolor(Light *light, float r, float g, float b);
void lightcalclightmap(Light *light, struct World *world);
void lightclearlightmap(Light *light);
void lightapplytosegment(List *lights, struct Segment *seg);
void lightclearsegment(struct Segment *seg);
void lightsegmentcalcmap(struct World *, struct Segment *);
void lightdelete(Light *light);

#endif /* CAELUM_LIGHT_H */
