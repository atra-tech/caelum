/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved
 *
 *  Defines player and agent structures and functions.
 *
 * */

#ifndef CAELUM_AGENT_H
#define CAELUM_AGENT_H

#include <cglm/cglm.h>
#include "render.h"
#include "physics.h"

#define AGENT_VIEW_RADIUS 15
#define AGENT_VIEW_AREA (AGENT_VIEW_RADIUS * AGENT_VIEW_RADIUS)

struct World;

enum {
    SIGHT_VISIBLE,
    SIGHT_PARTIAL,
    SIGHT_BLOCKED
};

typedef struct Agent {
    Entity ent;
    vec3 vel;
    mat4 modelmat;
    VBuffer vbo;
    AABB2 aabb;
    vec3 color;
    int sightmap[AGENT_VIEW_AREA];
    //Inventory inv
} Agent;

void agentinit(void);
Agent *agentcreate(void);
int agentregister(Agent *agent, const char *id);
void agentsetplayer(Agent *agent);
void agentsetplayercam(Agent *agent);
Agent *agentget(const char *id);
Agent *agentgetplayer(void);
Agent *agentgetplayercam(void);
void agentsetpos(Agent *agent, float x, float y, float z);
void agentsetrot(Agent *agent, float x, float y, float z);
void agentderegister(const char *id);
void agentdelete(Agent *agent);
void agentshutdown(void);

void playerhandle(Event *e);
void playerupdate(struct World *world, float dt);
void playerrender(void);

#endif /* CAELUM_AGENT_H */
