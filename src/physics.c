/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Defines types and functions for physics and
 *  collision calculations.
 *
 * */

#include "physics.h"
#include "agent.h"

/* Returns 1 if the two squares are intersecting. Otherwise, returns 0 */
int
aabb2intersects(AABB2 *box_a, AABB2 *box_b)
{
    if (box_a->x1 < box_b->x2 && box_a->x2 > box_b->x1 &&
            box_a->y1 < box_b->y2 && box_a->y2 > box_b->y1) {
        return 1;
    }

    return 0;
}

/* Returns 1 if the two boxes are intersecting.  Otherwise, returns 0 */
int
aabb3intersects(AABB3 *box_a, AABB3 *box_b)
{
    if (box_a->x1 < box_b->x2 && box_a->x2 > box_b->x1 &&
            box_a->y1 < box_b->y2 && box_a->y2 > box_b->y1 &&
            box_a->z1 < box_b->z2 && box_a->z2 > box_b->z1 ) {
        return 1;
    }

    return 0;
}

/* Repositions agent to prevent intersection with box.
 * NOTE: Must be applied after velocity is set and before pos is updated. */
void
agentcollide(Agent *agent, AABB2 *box)
{
    AABB2 *agent_box = &agent->aabb;
    float dx, dy;
    float cx_a, cx_b, cy_a, cy_b;
    float overlap;

    if (!aabb2intersects(agent_box, box)) {
        return;
    }

    cx_a = (agent_box->x1 + agent_box->x2) * 0.5f;
    cy_a = (agent_box->y1 + agent_box->y2) * 0.5f;
    cx_b = (box->x1 + box->x2) * 0.5f;
    cy_b = (box->y1 + box->y2) * 0.5f;

    dx = cx_a - cx_b;
    dy = cy_a - cy_b;

    if (fabs(dx) < fabs(dy)) {
        overlap = (cy_a - agent_box->y1) + (cy_b - box->y1) - fabs(dy);

        if (dy < 0) {
            agent->ent.z -= overlap;
            agent->aabb.y1 -= overlap;
            agent->aabb.y2 -= overlap;
        } else {
            agent->ent.z += overlap;
            agent->aabb.y1 += overlap;
            agent->aabb.y2 += overlap;
        }
    } else if (fabs(dx) > fabs(dy)) {
        overlap = (cx_a - agent_box->x1) + (cx_b - box->x1) - fabs(dx);

        if (dx < 0) {
            agent->ent.x -= overlap;
            agent->aabb.x1 -= overlap;
            agent->aabb.x2 -= overlap;
        } else {
            agent->ent.x += overlap;
            agent->aabb.x1 += overlap;
            agent->aabb.x2 += overlap;
        }
    }
}
