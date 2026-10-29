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

    player->aabb.x1 = player->ent.x - 0.2;
    player->aabb.x2 = player->ent.x + 0.2;
    player->aabb.y1 = player->ent.z - 0.2;
    player->aabb.y2 = player->ent.z + 0.2;

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

    y = 0;

    for (x = 512; x < 512 + SEGMENT_SIZE; x++) {
        for (z = 512; z < 512 + SEGMENT_SIZE; z++) {
            if (x == 512 || z == 512 ||
                    x == 512 + SEGMENT_SIZE - 1 ||
                    z == 512 + SEGMENT_SIZE - 1) {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("int_wall_lamp");
            } else {
                tile = worldgettoptileat(world, x, y, z);
                tile->data = tilegetdata("int_floor_tiled");
            }
        }
    }
    TileData *wall = tilegetdata("int_wall_lamp");
    worldgettoptileat(world, 522, 0, 518)->data = wall;
    worldgettoptileat(world, 522, 0, 519)->data = wall;
    worldgettoptileat(world, 522, 0, 520)->data = wall;
    worldgettoptileat(world, 522, 0, 521)->data = wall;
    worldgettoptileat(world, 516, 0, 516)->data = wall;
    worldgettoptileat(world, 517, 0, 516)->data = wall;
    worldgettoptileat(world, 518, 0, 516)->data = wall;
    worldgettoptileat(world, 519, 0, 516)->data = wall;
    worldgettoptileat(world, 520, 0, 516)->data = wall;
    worldgettoptileat(world, 524, 0, 527)->data = tilegetdata("int_floor_tiled");

    worldaddlight(world, 515.5, 0, 516.5, 0.4, 0.8, 1.0, 8);
    worldaddlight(world, 524.5, 0, 526.5, 1.0, 0.8, 0.4, 8);
    
    worldaddlight(world, 504.5, 0, 536.5, 1.0, 0.8, 0.1, 8);
    worldaddlight(world, 504.5, 0, 504.5, 0.4, 1.0, 0.1, 8);
    worldaddlight(world, 536.5, 0, 504.5, 0.1, 0.8, 1.0, 8);
    worldaddlight(world, 536.5, 0, 536.5, 1.0, 0.1, 0.8, 8);
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

void
mainstatestart(void)
{
    atrapushstate(caelumstate);
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);
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
    worldupdate(world, dt);
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
