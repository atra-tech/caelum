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

typedef struct Agent {
    Entity ent;
    vec3 vel;
    mat4 modelmat;
    VBuffer vbo;
    //Inventory inv
} Agent;

void agentinit(void);
Agent *agentcreate(void);
int agentregister(Agent *agent, const char *id);
void agentsetplayer(Agent *agent);
Agent *agentget(const char *id);
Agent *agentgetplayer(void);
void agentsetpos(Agent *agent, float x, float y, float z);
void agentsetrot(Agent *agent, float x, float y, float z);
void agentderegister(const char *id);
void agentdelete(Agent *agent);
void agentshutdown(void);

#endif /* CAELUM_AGENT_H */
