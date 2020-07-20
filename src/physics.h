/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Defines types and functions for physics and
 *  collision calculations.
 *
 * */

#ifndef CAELUM_PHYSICS_H
#define CAELUM_PHYSICS_H

#include "util/log.h"

struct Agent;

typedef struct AABB2 {
    float x1, y1;
    float x2, y2;
} AABB2;

typedef struct AABB3 {
    float x1, y1, z1;
    float x2, y2, z2;
} AABB3;

int aabb2intersects(AABB2 *box_a, AABB2 *box_b);
int aabb3intersects(AABB3 *box_a, AABB3 *box_b);
void aabb2collide(AABB2 *box_a, AABB2 *box_b);
void aabb3collide(AABB3 *box_a, AABB3 *box_b);
void agentcollide(struct Agent *agent, AABB2 *box);

#endif /* CAELUM_PHYSICS_H */
