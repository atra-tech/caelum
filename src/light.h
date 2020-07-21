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

typedef struct World World;

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
void lightcalclightmap(Light *light, World *world);
void lightapplylight(Light *light, World *world);
void lightremovelight(Light *light, World *world);
void lightdelete(Light *light);

#endif /* CAELUM_LIGHT_H */
