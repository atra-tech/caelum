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
/*
#define COLOR_GET_R(c) ((c >> 16) & 0xFF)
#define COLOR_GET_G(c) ((c >> 8) & 0xFF)
#define COLOR_GET_B(c) ((c >> 0) & 0xFF)

#define COLOR_SET_R(c, x) ((c & 0xFF00FFFF) | (x << 16))
#define COLOR_SET_G(c, x) ((c & 0xFFFF00FF) | (x << 8))
#define COLOR_SET_B(c, x) ((c & 0xFFFFFF00) | (x << 0))

#define COLOR_ADD_R(c, x) ((x << 16) | c)
#define COLOR_ADD_G(c, x) ((x << 8) | c)
#define COLOR_ADD_B(c, x) ((x << 0) | c)
#define COLOR_ADD(c, x) (c | x)
*/
/*
typedef struct LightVec {
    float x, y, z;
    float r, g, b, a;
} LightColor;
*/

typedef struct World World;

typedef struct LightColor {
    float r, g, b;
} LightColor;

typedef struct Light {
    float x, y, z;
    LightColor color;
    uint radius;
    LightColor *lightmap;
} Light;

Light * lightcreate(float x, float y, float z, uint radius);
void lightsetcolor(Light *light, float r, float g, float b);
void lightcalclightmap(Light *light, World *world);
void lightapplylight(Light *light, World *world);
void lightremovelight(Light *light, World *world);
void lightdelete(Light *light);

#endif /* CAELUM_LIGHT_H */
