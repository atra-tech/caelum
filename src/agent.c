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
#include "physics.h"
#include "light.h"

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
static void agentcalccolor(Agent *agent, World *world);
static void agentworldcollide(Agent *agent, World *world);

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
    //TODO: Send event data to inventory/GUI system.
    if (e->type == SDL_MOUSEWHEEL) {
        
    }
}

void
playerupdate(World *world, float dt)
{
    float speed;
    vec3 dt_vec = {dt, dt, dt};

    glm_vec3_zero(player->vel);
    speed = 4.0f;

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

    if (player->ent.x < 1.001) {
        player->ent.x = 1.001;
    } else if (player->ent.x > WORLD_SIZE - 1.001) {
        player->ent.x = WORLD_SIZE - 1.001;
    }
    if (player->ent.z < 1.001) {
        player->ent.z = 1.001;
    } else if (player->ent.z > WORLD_SIZE - 1.001) {
        player->ent.z = WORLD_SIZE - 1.001;
    }

    player->ent.x += player->vel[0];
    player->ent.y += player->vel[1];
    player->ent.z += player->vel[2];
    player->aabb.x1 += player->vel[0];
    player->aabb.y1 += player->vel[2];
    player->aabb.x2 += player->vel[0];
    player->aabb.y2 += player->vel[2];

    agentworldcollide(player, world);
    agentcalccolor(player, world);

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
    glm_translate_y(player->modelmat, player->ent.y);
    glm_translate_z(player->modelmat, player->ent.z);

    texsetactivetexture(0);
    texbind(tilegetalbedotex());

    shaderbind(&agentshader);
    shaderloadprojviewmat(&agentshader, getmaincamera()->projviewmat);
    shaderloadviewmat(&agentshader, getmaincamera()->viewmat);
    shaderloadmodelmat(&agentshader, player->modelmat);

    shaderaddvec3(&agentshader, "color", player->color);

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
    TileData *data = tilegetdata("maint_floor");

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

void
agentcalccolor(Agent *agent, World *world)
{
    uint x, y, z;
    LightColor *color;

    x = agent->ent.x;
    y = agent->ent.y + 0.5f;
    z = agent->ent.z;

    agent->color[0] = 0.0f;
    agent->color[1] = 0.0f;
    agent->color[2] = 0.0f;

    color = worldgetlightcellat(world, x, y, z);
    agent->color[0] += color->r;
    agent->color[1] += color->g;
    agent->color[2] += color->b;

    color = worldgetlightcellat(world, x + 1, y, z);
    agent->color[0] += color->r;
    agent->color[1] += color->g;
    agent->color[2] += color->b;

    color = worldgetlightcellat(world, x - 1, y, z);
    agent->color[0] += color->r;
    agent->color[1] += color->g;
    agent->color[2] += color->b;

    color = worldgetlightcellat(world, x, y, z + 1);
    agent->color[0] += color->r;
    agent->color[1] += color->g;
    agent->color[2] += color->b;

    color = worldgetlightcellat(world, x, y, z - 1);
    agent->color[0] += color->r;
    agent->color[1] += color->g;
    agent->color[2] += color->b;

    agent->color[0] *= 0.2f;
    agent->color[1] *= 0.2f;
    agent->color[2] *= 0.2f;
}

void
agentworldcollide(Agent *agent, World *world)
{
    int i, j;
    uint nx, ny, nz;
    Tile *tile;
    AABB2 box;

    nx = agent->ent.x;
    ny = agent->ent.y + 0.5;
    nz = agent->ent.z;

    tile = worldgettoptileat(world, nx, ny, nz);

    if (tile->data) {
        agent->ent.y = ny;
    } else {
        tile = worldgetsubtileat(world, nx, ny, nz);
        if (tile->data) {
            agent->ent.y = ny - 0.17;
        }
    }
    
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) { 
            tile = worldgettoptileat(world, nx + i, ny, nz + j);

            if (tile->data && tile->data->attr & TILE_WALL) {
                box = (AABB2){nx + i, nz + j, nx + i + 1, nz + j + 1};
                agentcollide(agent, &box);
            }
        }
    }
}
/*
void
agentworldcollide(Agent *agent, World *world)
{
    uint nx, ny, nz;
    float dx, dz, pad, invpad;
    Tile *tile;

    nx = agent->ent.x;
    ny = agent->ent.y + 0.5;
    nz = agent->ent.z;
    dx = agent->ent.x - nx;
    dz = agent->ent.z - nz;
    pad = 0.20f;
    invpad = 1 - pad;

    tile = worldgettoptileat(world, nx, ny, nz);

    if (tile->data) {
        agent->ent.y = ny;
    } else {
        tile = worldgetsubtileat(world, nx, ny, nz);
        if (tile->data) {
            agent->ent.y = ny - 0.17;
        }
    }
    tile = worldgettoptileat(world, nx - 1, ny, nz);

    if (dx < pad && tile->data) {
        if (tile->data->attr & TILE_WALL) {
            agent->ent.x = nx + pad;
        }
    }    
    tile = worldgettoptileat(world, nx + 1, ny, nz);

    if (dx > invpad && tile->data) {
        if (tile->data->attr & TILE_WALL) {
            agent->ent.x = nx + invpad;
        }
    }
    tile = worldgettoptileat(world, nx, ny, nz - 1);

    if (dz < pad && tile->data) {
        if (tile->data->attr & TILE_WALL) {
            agent->ent.z = nz + pad;
        }
    }
    tile = worldgettoptileat(world, nx, ny, nz + 1);

    if (dz > invpad && tile->data) {
        if (tile->data->attr & TILE_WALL) {
            agent->ent.z = nz + invpad;
        }
    }
}*/
