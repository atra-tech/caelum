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
#include "util/log.h"
#include "agent.h"
#include "input.h"
#include "world.h"
#include "core.h"

static const float playerverts[] = {
    -0.2f,  0.0f,  0.2f,
     0.2f,  0.0f,  0.2f,
     0.2f,  0.8f,  0.2f,
    -0.2f,  0.8f,  0.2f,

     0.2f,  0.0f, -0.2f,
    -0.2f,  0.0f, -0.2f,
    -0.2f,  0.8f, -0.2f,
     0.2f,  0.8f, -0.2f,

    -0.2f,  0.0f, -0.2f,
    -0.2f,  0.0f,  0.2f,
    -0.2f,  0.8f,  0.2f,
    -0.2f,  0.8f, -0.2f,

     0.2f,  0.0f,  0.2f,
     0.2f,  0.0f, -0.2f,
     0.2f,  0.8f, -0.2f,
     0.2f,  0.8f,  0.2f,

    -0.2f,  0.8f,  0.2f,
     0.2f,  0.8f,  0.2f,
     0.2f,  0.8f, -0.2f,
    -0.2f,  0.8f, -0.2f
};

static const float playernorms[] = {
     0.0f,  0.0f,  1.0f,
     0.0f,  0.0f, -1.0f,
    -1.0f,  0.0f,  0.0f,
     1.0f,  0.0f,  0.0f,
     0.0f,  1.0f,  0.0f,
};

static void playermesh(void);

static Agent *player;
static Agent *playercam;
static List *agentlist;
static HTable *agenttable;

static int playermeshed;
static Shader agentshader;

void
agentinit(void)
{
    player = NULL;
    agentlist = listcreate();
    agenttable = htablecreate(htablestringhash, htablestringequal);

    playermeshed = 0;
    shaderinit(&agentshader, 
            "res/shader/agentvert.glsl", 
            "res/shader/agentfrag.glsl");
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

void
agentsetplayercam(Agent *agent)
{
    playercam = agent;
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

Agent *
agentgetplayercam(void)
{
    return playercam;
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

void
playerhandle(Event *e)
{
/*    float speed = 10.0;
    glm_vec3_zero(player->vel);

    if (keystate(KEYCODE(w))) {
        player->vel[0] = speed;
    }
    if (keystate(KEYCODE(s))) {
        player->vel[0] = -speed;
    }
    if (keystate(KEYCODE(d))) {
        player->vel[2] = speed;
    }
    if (keystate(KEYCODE(a))) {
        player->vel[2] = -speed;
    }*/
}

void
playerupdate(World *world, float dt)
{
    vec3 dt_vec = {dt, dt, dt};
    float speed = 4.0;
    Tile *tile;

    glm_vec3_zero(player->vel);

    if (keystate(KEYCODE(w))) {
        player->vel[0] = speed;
    }
    if (keystate(KEYCODE(s))) {
        player->vel[0] = -speed;
    }
    if (keystate(KEYCODE(d))) {
        player->vel[2] = speed;
    }
    if (keystate(KEYCODE(a))) {
        player->vel[2] = -speed;
    }

    glm_vec3_mul(player->vel, dt_vec, player->vel);

    player->ent.x += player->vel[0];
    player->ent.y += player->vel[1];
    player->ent.z += player->vel[2];

    if (player->ent.x < 0.001) {
        player->ent.x = 0.001;
    } else if (player->ent.x > WORLD_SIZE - 0.001) {
        player->ent.x = WORLD_SIZE - 0.001;
    }
    if (player->ent.z < 0.001) {
        player->ent.z = 0.001;
    } else if (player->ent.z > WORLD_SIZE - 0.001) {
        player->ent.z = WORLD_SIZE - 0.001;
    }

    // TODO: Collide
    // AABB Collision box for player.  Compare against surrounding tiles.
    

    logprint(LOG_INFO, "Player pos: x:$f, z:$f", player->ent.x, player->ent.z);

    playercam->ent.x = player->ent.x - 6;
    playercam->ent.y = player->ent.y + 16;
    playercam->ent.z = player->ent.z;
}

void
playerrender(void)
{
    if (!playermeshed) {
        playermesh();
        playermeshed = 1;
    }
    
    glm_mat4_identity(player->modelmat);
    glm_translate_x(player->modelmat, player->ent.x);
    glm_translate_z(player->modelmat, player->ent.z);

    texsetactivetexture(0);
    texbind(tilegetalbedotex());

    shaderbind(&agentshader);
    shaderloadprojviewmat(&agentshader, getmaincamera()->projviewmat);
    shaderloadviewmat(&agentshader, getmaincamera()->viewmat);
    shaderloadmodelmat(&agentshader, player->modelmat);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(player->vbo.vao);
    glDrawElements(GL_TRIANGLES, player->vbo.indxcount, GL_UNSIGNED_INT, NULL);
}

void
playermesh(void)
{
    uint i, j, vertidx, uvidx, normidx, indexcount;
    vec3 vert;
    vec2 uv;
    vec3 norm;
    ivec3 index;
    TileData *data = tilegetdata("res/tile/maint_floor.abf");

    indexcount = player->vbo.indxcount / 3;

    vbufclear(&player->vbo);

    for (i = 0; i < 5; i++) {
        normidx = i * 3;

        for (j = 0; j < 4; j++) {
            vertidx = i * 12 + j * 3;
            uvidx = j * 2;
            vert[0] = playerverts[vertidx + 0];
            vert[1] = playerverts[vertidx + 1];
            vert[2] = playerverts[vertidx + 2];

            uv[0] = data->toptex[uvidx + 0];
            uv[1] = data->toptex[uvidx + 1];

            norm[0] = playernorms[normidx + 0];
            norm[1] = playernorms[normidx + 1];
            norm[2] = playernorms[normidx + 2];

            vbufaddvertex(&player->vbo, vert, uv, norm);
        }
        index[0] = indexcount + 0;
        index[1] = indexcount + 1;
        index[2] = indexcount + 2;
        vbufaddindex(&player->vbo, index);
        
        index[0] = indexcount + 2;
        index[1] = indexcount + 3;
        index[2] = indexcount + 0;
        vbufaddindex(&player->vbo, index);

        indexcount += 4;
        player->vbo.indxcount += 12;
    }

    vbufbind(&player->vbo);
}
