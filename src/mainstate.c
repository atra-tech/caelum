/*  
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Holds the state information and functions for the main gameplay state.
 *  This is where input, updates and rendering are managed.
 *
 * */

#include "util/log.h"
#include "mainstate.h"
#include "engine.h"
#include "core.h"
#include "world.h"
#include "agent.h"
#include "light.h"

static void caelumhandle(void);
static void caelumupdate(float dt);
static void caelumrender(void);
static void createtestworld(void);

static Camera *camera;
static State *caelumstate;
static World *world;
static Agent *player;
static Agent *playercam;
//static 

void
mainstateinit(void)
{
    caelumstate = statecreate(caelumhandle, caelumupdate, caelumrender);
    camera = getmaincamera();
    worldinit();
    agentinit();
    world = worldcreate();

    player = agentcreate();
    playercam = agentcreate();
    
    player->ent.x = 520.0f;
    player->ent.y = 0.0f;
    player->ent.z = 520.0f;
    player->ent.rx = 0.00f;
    player->ent.ry = 0.00f;
    player->ent.rz = 0.00f;

    playercam->ent.x = 2.0f;
    playercam->ent.y = 16.0f;
    playercam->ent.z = 8.0f;
    playercam->ent.rx = 5.06f;
    playercam->ent.ry = 1.57f;
    playercam->ent.rz = 0.00f;

    agentsetplayer(player);
    agentsetplayercam(playercam);
    camerabindtoentity(camera, &playercam->ent);
    createtestworld();
}

/* TEST FUNCTION */
void
createtestworld(void)
{
    uint x, y, z;
    Tile *tile;
    Light *light;

    y = 0;

    for (x = 512; x < 512 + SEGMENT_SIZE; x++)
    {
        for (z = 512; z < 512 + SEGMENT_SIZE; z++)
        {
            if (x == 512 || z == 512 ||
                    x == 512 + SEGMENT_SIZE - 1 ||
                    z == 512 + SEGMENT_SIZE - 1) {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("res/tile/int_wall.abf");
            } else {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("res/tile/int_floor_tiled.abf");
            }
        }
    }
    TileData *wall = tilegetdata("res/tile/int_wall_lamp.abf");
    worldgettoptileat(world, 522, 0, 518)->data = wall;
    worldgettoptileat(world, 522, 0, 519)->data = wall;
    worldgettoptileat(world, 522, 0, 520)->data = wall;
    worldgettoptileat(world, 522, 0, 521)->data = wall;
    worldgettoptileat(world, 516, 0, 516)->data = wall;
    worldgettoptileat(world, 517, 0, 516)->data = wall;
    worldgettoptileat(world, 518, 0, 516)->data = wall;
    worldgettoptileat(world, 519, 0, 516)->data = wall;
    worldgettoptileat(world, 520, 0, 516)->data = wall;
    worldgettoptileat(world, 524, 0, 527)->data =
        tilegetdata("res/tile/int_floor_tiled.abf");

    light = lightcreate(515.0, 0.5, 516.0, 10);
    lightsetcolor(light, 0.1, 0.5, 1.0);
    lightcalclightmap(light, world);
    lightapplylight(light, world);
    light = lightcreate(524.0, 0.5, 526.0, 14);
    //light = lightcreate(3.0, 0.5, 3.0, 10);
    lightsetcolor(light, 1.0, 0.5, 0.1);
    lightcalclightmap(light, world);
    lightapplylight(light, world);
}

void
mainstatestart(void)
{
    atrapushstate(caelumstate);
}

void
mainstatestop(void)
{
    State *state = statetop();

    if (state != caelumstate) {
        logprint(LOG_ERROR, "mainstate: Unable to pop main state: " \
                "State not present at top of state stack");
        return;
    }
    worlddelete(world);
    atrapopstate();
}

void
caelumhandle(void)
{
    Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            stopatraengine();
            exit(EXIT_SUCCESS);
        }
        windoweventhandle(getmainwindow(), &e, camera);
        playerhandle(&e);
        worldhandle(world, &e);
        // Pass event to player for handling
        // playerhandle(&e);
    }
}

void
caelumupdate(float dt)
{
    // Update game elements and GUI
    // Update player position based on velocity.
    playerupdate(world, dt * 0.001);
    cameraupdate(camera);
}

void
caelumrender(void)
{   
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    // Render players, world, entities, etc.
    worldrender(world, camera);
    playerrender();
}
