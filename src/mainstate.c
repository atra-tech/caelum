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

static void caelumhandle(void);
static void caelumupdate(float dt);
static void caelumrender(void);

static State *caelumstate;

void
mainstateinit(void)
{
    caelumstate = statecreate(caelumhandle, caelumupdate, caelumrender);
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
    atrapopstate();
}

void
caelumhandle(void)
{
    Event e;

    // Handle input
    logprint(LOG_INFO, "CAELUM: Handled");

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            stopatraengine();
            exit(EXIT_SUCCESS);
        }
    }
}

void
caelumupdate(float dt)
{
    // Update game elements and GUI

    logprint(LOG_INFO, "CAELUM: Updated: dt: $f", dt);
}

void
caelumrender(void)
{
    // Render players, world, entities, etc.
    logprint(LOG_INFO, "CAELUM: Rendered");
}
