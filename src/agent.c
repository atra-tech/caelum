/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved
 *
 *  Defines player and agent structures and functions.
 *
 * */

#include "util/htable.h"
#include "util/list.h"
#include "agent.h"

static Agent *player;
static List *agentlist;
static HTable *agenttable;

void
agentinit(void)
{
    player = NULL;
    agentlist = listcreate();
    agenttable = htablecreate(htablestringhash, htablestringequal);
}

Agent *
agentcreate(void)
{
    Agent *agent = calloc(1, sizeof(Agent));

    vbufinit(&agent->vbo);
    listpushback(agentlist, agent);

    return agent;
}

/* Register agent with an ID which can be used to retrieve it later */
int
agentregister(Agent *agent, const char *id)
{
    char *key = malloc((strlen(id) + 1) * sizeof(char));
    
    strcpy(key, id);

    return htableadd(agenttable, key, agent);
}

void
agentsetplayer(Agent *agent)
{
    player = agent;
}

/* Returns an agent with a matching ID */
Agent *
agentget(const char *id)
{
    return htableget(agenttable, id);
}

Agent *
agentgetplayer(void)
{
    return player;
}

void
agentsetpos(Agent *agent, float x, float y, float z)
{
    agent->ent.x = x;
    agent->ent.y = y;
    agent->ent.z = z;
}

void
agentsetrot(Agent *agent, float x, float y, float z)
{
    agent->ent.rx = x;
    agent->ent.ry = y;
    agent->ent.rz = z;
}

void
agentderegister(const char *id)
{
    htableremove(agenttable, id);
}

void
agentdelete(Agent *agent)
{
    uint i;
    vbufdelete(&agent->vbo);

    for (i = 0; i < agentlist->size; i++) {
        if (agent == agentlist->elem[i]) {
            listremove(agentlist, i);
        }
    }
    free(agent);
}

void
agentshutdown(void)
{
    listdelete(agentlist);
    htabledelete(agenttable);
}
