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
    player->ent.x = 4.0f;
    player->ent.y = 10.0f;
    player->ent.z = 20.0f;
    player->ent.rx = 5.06f;//5.23f;
    player->ent.ry = 1.57f;
    player->ent.rz = 0.00f;
    agentsetplayer(player);
    camerabindtoentity(camera, &player->ent);
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

    for (x = 0; x < SEGMENT_SIZE; x++)
    {
        for (z = 0; z < SEGMENT_SIZE; z++)
        {
            if (x == 0 || z == 0 || x == SEGMENT_SIZE - 1 || z == SEGMENT_SIZE - 1) {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("res/tile/int_wall.abf");
            } else {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("res/tile/int_floor_tiled.abf");
            }
        }
    }
    TileData *wall = tilegetdata("res/tile/int_wall_lamp.abf");
    worldgettoptileat(world, 10, 0, 6)->data = wall;
    worldgettoptileat(world, 10, 0, 7)->data = wall;
    worldgettoptileat(world, 10, 0, 8)->data = wall;
    worldgettoptileat(world, 10, 0, 9)->data = wall;
    worldgettoptileat(world, 4, 0, 4)->data = wall;
    worldgettoptileat(world, 5, 0, 4)->data = wall;
    worldgettoptileat(world, 6, 0, 4)->data = wall;
    worldgettoptileat(world, 7, 0, 4)->data = wall;
    worldgettoptileat(world, 8, 0, 4)->data = wall;
    worldgettoptileat(world, 12, 0, 15)->data =
        tilegetdata("res/tile/int_floor_tiled.abf");

    light = lightcreate(3.0, 0.5, 4.0, 10);
    lightsetcolor(light, 0.1, 0.5, 1.0);
    lightcalclightmap(light, world);
    lightapplylight(light, world);
    light = lightcreate(12.0, 0.5, 14.0, 8);
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
}
